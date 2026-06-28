#include <d3d11.h>
#include <dxgi1_3.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <windows.h>

class KodiLibplaceboPresenter {
private:
  std::thread m_presentThread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::atomic<bool> m_frameReady {false};
  std::atomic<bool> m_running {true};

  // Shared thread-safe clock for your PTS filter loop
  std::atomic<int64_t> m_lastVsyncTimestamp {0};

  IDXGISwapChain1* m_swapChain = nullptr;
  ID3D11DeviceContext* m_immediateContext = nullptr;
  ID3D11Multithread* m_d3dMultithread = nullptr;
  HANDLE m_waitableObject = nullptr;

  void PresentLoop() {
	// Boost this thread to time-critical priority so DWM/V-Sync wakes it instantly
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	while(m_running) {
	  std::unique_lock<std::mutex> lock(m_mutex);
	  m_cv.wait(lock, [this] { return m_frameReady.load() || !m_running; });

	  if(!m_running) break;
	  m_frameReady = false;
	  lock.unlock(); // Release CPU lock; let the rendering thread start the next frame

	  // 1. Hardware Synchronization
	  // Kernel-level sleep until the GPU finishes displaying the previous frame
	  if(m_waitableObject) {
		::WaitForSingleObject(m_waitableObject, INFINITE);
	  }

	  // 2. Thread Safety Safety Net
	  // Lock the immediate context so libplacebo doesn't conflict during swap
	  if(m_d3dMultithread) {
		m_d3dMultithread->Enter();
	  }

	  // 3. Execution
	  if(m_swapChain) {
		// Blocks smoothly on V-Sync here, entirely off the rendering thread
		m_swapChain->Present(1, 0);

		// 4. Capture the true V-Sync completion timestamp
		LARGE_INTEGER qpc;
		::QueryPerformanceCounter(&qpc);
		m_lastVsyncTimestamp.store(qpc.QuadPart, std::memory_order_release);
	  }

	  // 5. Release Context
	  if(m_d3dMultithread) {
		m_d3dMultithread->Leave();
	  }
	}
  }

public:
  KodiLibplaceboPresenter(IDXGISwapChain1* sc, ID3D11DeviceContext* ctx, HANDLE waitHandle)
	: m_swapChain(sc), m_immediateContext(ctx), m_waitableObject(waitHandle)
  {
	// Acquire the multithread manager from the immediate context
	if(SUCCEEDED(m_immediateContext->QueryInterface(IID_PPV_ARGS(&m_d3dMultithread)))) {
	  m_d3dMultithread->SetMultithreadProtected(TRUE);
	}

	m_presentThread = std::thread(&KodiLibplaceboPresenter::PresentLoop, this);
  }

  ~KodiLibplaceboPresenter() {
	m_running = false;
	m_cv.notify_one();
	if(m_presentThread.joinable()) {
	  m_presentThread.join();
	}
	if(m_d3dMultithread) m_d3dMultithread->Release();
  }

  // Called by the Rendering Thread right after libplacebo submissions finish
  void SignalFrameReady() {
	if(m_immediateContext) {
	  // Push all libplacebo shaders out of the CPU driver directly to the GPU hardware
	  m_immediateContext->Flush();
	}

	{
	  std::lock_guard<std::mutex> lock(m_mutex);
	  m_frameReady = true;
	}
	m_cv.notify_one(); // Instantly wake up our presentation thread
  }

  // Called by Kodi's filtering loop to get the highly accurate V-Sync timing
  int64_t GetLatestVsyncTime() const {
	return m_lastVsyncTimestamp.load(std::memory_order_acquire);
  }
};