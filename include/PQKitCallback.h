#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include "rpc.tlh"
#include "PQKitCallBackBase.h"
#include <QObject>


class CPQKitCallback : public QObject, public CPQKitCallbackBase
{
	Q_OBJECT

public:
	CPQKitCallback(QObject* parent = nullptr);
	~CPQKitCallback();


//
//IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr);

	STDMETHOD(QueryInterface)(const struct _GUID &iid, void ** ppv);
	ULONG __stdcall AddRef(void);
	ULONG __stdcall Release(void);

public:
	//
	HRESULT Fire_Initialize_Result(long lResult);
	HRESULT Fire_RunCMD_Result(long lResult);
	HRESULT Fire_GetData_Result(long lResult);
	HRESULT Notify_Raise_Dockwindow(int i_nType);
	HRESULT Fire_Login_Result(int i_nLoginType);
	HRESULT Fire_Path_Generate_Result(long i_bSuccess, int i_nPathCount, int i_nIndex, unsigned long i_ulPathID);
	HRESULT Fire_Element_Pickup(unsigned long i_ulObjID, LPWSTR i_wsEntityName, int i_nEntityType,
		double i_dPointX, double i_dPointY, double i_dPointZ);
	HRESULT Fire_RButton_Up(long i_lPosX,long i_lPosY);
	HRESULT Fire_LButton_Up(long i_lPosX,long i_lPosY);
	HRESULT Fire_Menu_Pop(unsigned long i_ulObjID,long i_lPosX,long i_lPosY,int *o_nHandled);
	HRESULT Fire_Draw();

signals:                
	void signalInitializeResult(long lResult);
	void signalRunCMDResult(long lResult);
	void signalGetDataResult(long lResult);
	void signalRaiseDockwindow(int i_nType);
	void signalLoginResult(int i_nLoginType);
	void signalPathGenerateResult(long i_bSuccess, int i_nPathCount, int i_nIndex, unsigned long i_ulPathID);
	void signalElementPickup(unsigned long i_ulObjID, LPWSTR i_wsEntityName, int i_nEntityType, 
		double i_dPointX, double i_dPointY, double i_dPointZ);
	void signalRButtonUp(long i_lPosX,long i_lPosY);
	void signalLButtonUp(long i_lPosX,long i_lPosY);
	void signalMenuPop(unsigned long i_ulObjID,long i_lPosX,long i_lPosY,int *o_nHandled);
	void signalDraw();
};
