//#pragma once
//#include "Application.h"
//#include <string>
//#include <Windows.h>
//#include <memory>
//#include "Scene.h"
//
//using namespace std;
//namespace Engine {
//
//	class ApplicationWindows :
//		public Application, public Singleton<ApplicationWindows>
//	{
//		friend class Singleton<ApplicationWindows>;
//		friend class Application;
//	public:
//		HINSTANCE hInstance;
//		ApplicationWindows();
//		ApplicationWindows(LPCWSTR szWindowClass, LPCWSTR szTitle, int nCmdShow, WNDCLASSEXW wcex);
//		bool Initialize() override;
//		int Run() override;
//		void Shutdown() override;
//
//		static unique_ptr<ApplicationWindows> instance;
//	};
//
//}
