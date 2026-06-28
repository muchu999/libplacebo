#include <d3d11.h>
#include <dxgi1_3.h> 
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <windows.h>

class KodiFlipPresenter {
private:
  std::thread m_presentThread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::atomic<bool> m_frameReady {false};
  std::atomic<bool> m_running {true};

  IDXGISwapChain1* m_swapChain = nullptr;
  HANDLE m_frameLatencyWaitableObject = nullptr;

  void PresentLoop() {
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// Retrieve the native D3D11 multithread interface from your immediate context
	Microsoft::WRL::ComPtr<ID3D11Multithread> pMultithread;
	if(FAILED(m_context->QueryInterface(IID_PPV_ARGS(&pMultithread)))) {
	  CLog::LogF(LOGERROR, "Failed to get ID3D11Multithread interface.");
	  return;
	}

	while(m_running) {
	  std::unique_lock<std::mutex> lock(m_mutex);
	  m_cv.wait(lock, [this] { return m_frameReady.load() || !m_running; });

	  if(!m_running) break;
	  m_frameReady = false;
	  lock.unlock();

	  // 1. Waitable Object Handle: Clean kernel sleep until a presentation slot opens
	  if(m_frameLatencyWaitableObject) {
		::WaitForSingleObject(m_frameLatencyWaitableObject, INFINITE);
	  }

	  // 2. Protect the Immediate Context: 
	  // This temporarily blocks libplacebo on the rendering thread if it tries 
	  // to submit shaders at the exact microsecond of presentation.
	  pMultithread->Enter();

	  if(m_swapChain) {
		// 3. Execute presentation (V-Sync blocks safely here)
		m_swapChain->Present(1, 0);
	  }

	  // 4. Release the context back to libplacebo immediately
	  pMultithread->Leave();
	}
  }
public:
  // Pass your existing SwapChain here
  KodiFlipPresenter(IDXGISwapChain1* swapChain) : m_swapChain(swapChain) {
	// Query the swapchain to get the OS Waitable Object handle
	Microsoft::WRL::ComPtr<IDXGISwapChain2> swapChain2;
	if(SUCCEEDED(m_swapChain->QueryInterface(IID_PPV_ARGS(&swapChain2)))) {
	  m_frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();
	}

	m_presentThread = std::thread(&KodiFlipPresenter::PresentLoop, this);
  }

  ~KodiFlipPresenter() {
	m_running = false;
	m_cv.notify_one();
	if(m_presentThread.joinable()) {
	  m_presentThread.join();
	}
  }

  // Called by Kodi's main rendering thread after m_d3dContext->ExecuteCommandList(...)
  void SignalFrameReady(ID3D11DeviceContext* immediateContext) {
	if(immediateContext) {
	  // Flush forces the commands out of the CPU driver and down to the hardware
	  immediateContext->Flush();
	}

	{
	  std::lock_guard<std::mutex> lock(m_mutex);
	  m_frameReady = true;
	}
	m_cv.notify_one(); // Instantly wake up our high priority present thread
  }
};