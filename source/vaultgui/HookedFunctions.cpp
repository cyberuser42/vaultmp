#include "HookedFunctions.h"

#include "myDirectInput8.h"

#include "TextureHooking.h"

#include "global.h"

#include "Export.h"

IDirect3D9* (WINAPI *Direct3DCreate9_Original)(UINT SDKVersion);
HRESULT (WINAPI *DirectInput8Create_Original)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);

HMODULE (WINAPI *LoadLibrary_Original)(LPCTSTR lpFileName);

FARPROC (WINAPI *GetProcAddress_Original)(HMODULE hModule,LPCSTR lpProcName);
ATOM (WINAPI *RegisterClass_Original)(const WNDCLASS *lpWndClass);
LRESULT (CALLBACK *WindowProcedure_Original)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
HANDLE (WINAPI *CreateFile_Original)(LPCTSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);

BOOL (WINAPI *ReadFile_Original)(HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped);
HFILE (WINAPI *OpenFile_Original)(LPCSTR lpFileName,LPOFSTRUCT lpReOpenBuff,UINT uStyle);

HRESULT (WINAPI *D3DXCreateTextureFromFileInMemory_Original)(LPDIRECT3DDEVICE9 pDevice,LPCVOID pSrcData,UINT SrcDataSize,LPDIRECT3DTEXTURE9 *ppTexture);

extern myIDirect3DDevice9* gl_pmyIDirect3DDevice9;

HRESULT WINAPI DirectInput8Create_Hooked(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter)
{
	SendToLog("DirectInput8Create_Hooked called");
	HRESULT ret=DirectInput8Create_Original(hinst,dwVersion,riidltf,ppvOut,punkOuter);

	(*ppvOut)=new MyDirectInput8((IDirectInput8*)*ppvOut);

	return ret;
}

IDirect3D9* WINAPI Direct3DCreate9_Hooked(UINT SDKVersion)
{
	SendToLog("Direct3DCreate9_Hooked called");
	IDirect3D9* tmp;
	tmp=(*Direct3DCreate9_Original)(SDKVersion);
	gl_pmyIDirect3D9 = new myIDirect3D9(tmp);
	
	SendToLog("Returning DX Pointer");

	return gl_pmyIDirect3D9;
}

LONG OnApplicationCrash(LPEXCEPTION_POINTERS p)
{
	//Exception handler
	//char* exstr=ExceptionToString(p->ExceptionRecord);
	SendToLog("EXCEPTION!");
	SendToLog(ExceptionToString(p->ExceptionRecord));

	Debug::InspectCrash(p);

	STARTUPINFO info={sizeof(info)};
	PROCESS_INFORMATION processInfo;
	if (CreateProcess("SendFalloutLog.exe", "", NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		/*::WaitForSingleObject(processInfo.hProcess, INFINITE);*/
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	//system("SendFalloutLog.exe");
	//Got an exception!
    return EXCEPTION_EXECUTE_HANDLER;
}

HMODULE WINAPI LoadLibrary_Hooked(LPCTSTR lpFileName)
{
	string out="LoadLibrary_Hooked(";
	out+=lpFileName+string(")");
	SendToLog((char*)out.c_str());
	return LoadLibrary_Original(lpFileName);
}

FARPROC WINAPI GetProcAddress_Hooked(HMODULE hModule,LPCSTR lpProcName)
{
		string out="GetProcAddress_Hooked(";
		out+=lpProcName+string(")");
		SendToLog((char*)out.c_str());

		FARPROC tmp=GetProcAddress_Original(hModule,lpProcName);
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&OnApplicationCrash);  
		DBB("GetProcAddress_Hooked("<<lpProcName<<")");
		if(strcmp(lpProcName,"Direct3DCreate9")==0)
		{
			Direct3DCreate9_Original=(IDirect3D9* (WINAPI *)(UINT SDKVersion))tmp;
			SendToLog("Returning fake Direct3DCreate9 (hooked function)");
			return (FARPROC)Direct3DCreate9_Hooked;
		}
        return tmp;
}

ATOM WINAPI RegisterClass_Hooked(
  __in  const WNDCLASS *lpWndClass
)
{
	SendToLog("RegisterClass_Hooked called");
	WindowProcedure_Original=lpWndClass->lpfnWndProc;

	((WNDCLASS *)lpWndClass)->lpfnWndProc=CustomWindowProcedure;

	SendToLog("Returning RegisterClass_Original");
	return RegisterClass_Original(lpWndClass);
}

LRESULT CALLBACK CustomWindowProcedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	static string chatbox_text="";
	static bool chatting=false;
	static int maxV=0;
	static char buff[100];

	SendToLog("CustomWindowProcedure called");

	switch(message)
	{	
#ifdef USE_CEGUI
		case WM_MOUSEMOVE:
			if(chatting)
			{

				int xPos = GET_X_LPARAM(lparam); 
				int yPos = GET_Y_LPARAM(lparam);
				CEGUI::System::getSingleton().injectMousePosition(xPos,yPos);

			}
			gl_pmyIDirect3DDevice9->wnd->setText("Mouse move!");
		break;
#endif

		case WM_LBUTTONDOWN:
			if(chatting)
			{
#ifdef USE_CEGUI
				int xPos = GET_X_LPARAM(lparam); 
				int yPos = GET_Y_LPARAM(lparam);
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MouseButton::LeftButton);
#endif
			}
		break;

		case WM_LBUTTONUP:
			if(chatting)
			{
				#ifdef USE_CEGUI
				int xPos = GET_X_LPARAM(lparam); 
				int yPos = GET_Y_LPARAM(lparam);
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MouseButton::LeftButton);
#endif
			}
		break;

		case WM_LBUTTONDBLCLK:
			if(chatting)
			{
				#ifdef USE_CEGUI
				int xPos = GET_X_LPARAM(lparam); 
				int yPos = GET_Y_LPARAM(lparam);
				CEGUI::System::getSingleton().injectMouseButtonDoubleClick(CEGUI::MouseButton::LeftButton);
#endif
			}
		break;

		case WM_CHAR:
			switch((char)wparam)
			{
				case 0x08: 
                    
                    // Process a backspace. 
					if(chatting&&chatbox_text.length()>0)
					{
						chatbox_text.pop_back();
						#ifdef USE_CEGUI

						/*((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"))->setText(chatbox_text);
						((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"))->setCaratIndex(chatbox_text.length());*/
#endif
					}
                     
                    break; 
 
                case 0x0A: 
                    
                    // Process a linefeed. 
                     
                    break; 
 
                case 0x1B: 
                    
                    // Process an escape. 
					
					chatbox_text="";
					chatting=false;
					gData.chatting=false;
					gData.disableMouseInput=false;
					#ifdef USE_CEGUI
					CEGUI::MouseCursor::getSingleton().hide();
					//((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"))->setText(chatbox_text);
#endif

					gData.chatting=false;
                    
                    break; 
 
                case 0x09: 
                    
                    // Process a tab. 
                     
                    break; 
				
 
                case 0x0D: 
                    
                    // Process a carriage return. 
					if(chatting)
					{
						
						CEGUI::Editbox* edb = ((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"));

						if(edb->hasInputFocus())
						{

							CEGUI::String txt=edb->getText();
							edb->setText("");


							//chatQueue.push_back((char*)txt.c_str());
							Chatbox_AddToChat((char*)txt.c_str());

						
							/*CEGUI::FormattedListboxTextItem* itm=new CEGUI::FormattedListboxTextItem(chatbox_text,CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
							itm->setTextColours(0xFFFFFFFF);
							CEGUI::Listbox* listb=((CEGUI::Listbox*)CEGUI::WindowManager::getSingleton().getWindow("List Box"));
							listb->addItem(itm);
							while(listb->getItemCount() > D_MAX_CHAT_ENTRIES) {
								listb->removeItem(listb->getListboxItemFromIndex(0));
							}
							listb->ensureItemIsVisible(itm);*/

							chatting=false;
							chatbox_text="";

							gData.chatting=false;
							gData.disableMouseInput=false;
							CEGUI::MouseCursor::getSingleton().hide();
							gData.lastChatTextTick=GetTickCount();
						}
					}
                     
                    break; 

				default:

				if(chatting)
				{
					/*if(chatbox_text.length()<120)
					{
						chatbox_text+=(char)wparam;
						((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"))->setText(chatbox_text);
						((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"))->setCaratIndex(chatbox_text.length());
					}*/
					char t=(char)wparam;
					if(t>=32)//if((t>='a'&&t<='z')||(t>='A'&&t<='Z'))
					{
						CEGUI::System::getSingleton().injectChar(t);
					}
				}

				if(((char)wparam=='b'||(char)wparam=='B')&&!chatting)
				{

				}

				if(((char)wparam=='t'||(char)wparam=='T')&&!chatting)
				{
					chatting=!chatting;
					gData.chatting=true;
					gData.disableMouseInput=true;
					CEGUI::MouseCursor::getSingleton().show();
					CEGUI::Editbox* edb = ((CEGUI::Editbox*)CEGUI::WindowManager::getSingleton().getWindow("Edit Box"));
					edb->activate();
#ifdef USE_CEGUI
					//CEGUI::MouseCursor::getSingleton().show();
#endif
				}
				
			}
			
		break;

		case WM_KEYDOWN:

			switch((char)wparam)
			{
				case VK_ADD:
					
					break;
				case VK_SUBTRACT:
					
					break;
			}

			break;

	}
	return WindowProcedure_Original(hwnd,message,wparam,lparam);
}

BOOL WINAPI ReadFile_Hook(HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped)
{
	return ReadFile_Original(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
}

HFILE WINAPI OpenFile_Hook(LPCSTR lpFileName,LPOFSTRUCT lpReOpenBuff,UINT uStyle)
{
	return OpenFile_Original(lpFileName,lpReOpenBuff,uStyle);
}

HANDLE WINAPI CreateFile_Hook(LPCTSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile)
{
	return CreateFile_Original(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemory_Hook(LPDIRECT3DDEVICE9 pDevice,LPCVOID pSrcData,UINT SrcDataSize,LPDIRECT3DTEXTURE9 *ppTexture)
{
	char* lastTextureLoadedBackup=lastTextureLoaded;
	char tmp[300];
	char *tmp2=(char*)pSrcData;
	sprintf(tmp,"0x%x D3DXCreateTextureFromFileInMemory %s",_ReturnAddress(),lastTextureLoadedBackup);
	SendToLog(tmp);

	if(lastTextureLoadedBackup!=0)
	{
		if(gData.textureHookingDone)
			TextureHooking::hookTextureIfNecessary((char*)lastTextureLoadedBackup,(char**)&pSrcData,(int*)&SrcDataSize);
	}

	HRESULT ret=D3DXCreateTextureFromFileInMemory_Original(pDevice,pSrcData,SrcDataSize,ppTexture);
	if(lastTextureLoadedBackup!=0)
		TextureHooking::registerTexture((char*)lastTextureLoadedBackup,*ppTexture);

	
	/*unsigned int textureHash=createHash((const char*)pSrcData,SrcDataSize);
	sprintf(tmp,"textureDump/%u.png",textureHash);
	D3DXSaveTextureToFile(tmp,D3DXIFF_PNG,(*ppTexture),NULL);*/


	return ret;
}

char* lastTextureLoaded=0;
int loadTextureJmp=0xAA1070;

__declspec(naked) void loadTextureHook()
{
	_asm
	{
		mov lastTextureLoaded,ebx
		jmp loadTextureJmp
	}
}

/*Some mess with player pointer data (FNV)*/
/*
int playerPointer=0;
int playerPointerJmp=0x871E2E;

__declspec(naked) void playerPointerHook()
{
	_asm
	{
		mov playerPointer,ecx
		mov eax,[edx+0x000001d0]
		jmp playerPointerJmp
	}
}*/