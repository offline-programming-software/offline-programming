#pragma once
#include <atlbase.h>
#include <atlcom.h>

#include "rpc.tlh"

class CPQKitCallbackBase : public IPQPlatformComponentCallBack
{

public:
	CPQKitCallbackBase()
	{
		m_lRefCount = 1;
	}
	virtual ~CPQKitCallbackBase()
	{

	}

	//IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) 
	{
		*pctinfo = 1; 
		return S_OK;
	}
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) 
	{
		return DISP_E_UNKNOWNNAME; 
	}
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
		VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) 
	{
		return DISP_E_MEMBERNOTFOUND; 
	}

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) 
	{
		if (riid == IID_IUnknown || riid == IID_IDispatch) {
			*ppvObject = static_cast<IDispatch*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() 
	{
		return InterlockedIncrement(&m_lRefCount);
	}
	STDMETHODIMP_(ULONG) Release() 
	{
		ULONG ulRefCount = InterlockedDecrement(&m_lRefCount);
		if (ulRefCount == 0)
		{
			delete this;
		}
		return ulRefCount;
	}


	//PQKit CallBack
	HRESULT Fire_Initialize_Result(long lResult)
	{
		return S_OK;
	}

	HRESULT Fire_RunCMD_Result(long lResult)
	{
		return S_OK;
	}

	HRESULT Fire_GetData_Result(long lResult)
	{
		return S_OK;
	}

	HRESULT Notify_Raise_Dockwindow(int i_nType)
	{
		return S_OK;
	}

	HRESULT Fire_Login_Result(int i_nLoginType)
	{
		return S_OK;
	}

	HRESULT Fire_Path_Generate_Result(long i_bSuccess, int i_nPathCount, int i_nIndex, unsigned long i_ulPathID)
	{
		return S_OK;
	}

	HRESULT Fire_Element_Pickup(unsigned long i_ulObjID, LPWSTR i_wsEntityName, int i_nEntityType, double i_dPointX, double i_dPointY, double i_dPointZ)
	{
		return S_OK;
	}

	HRESULT Fire_RButton_Up(long i_lPosX, long i_lPosY)
	{
		return S_OK;
	}

	HRESULT Fire_LButton_Up(long i_lPosX, long i_lPosY)
	{
		return S_OK;
	}

	HRESULT Fire_Menu_Pop(unsigned long i_ulObjID, long i_lPosX, long i_lPosY, int* o_nHandled)
	{
		return S_OK;
	}

	HRESULT Fire_Draw()
	{
		return S_OK;
	}

	HRESULT Fire_Element_Selection(LPWSTR i_wFaceNames, LPWSTR i_wEntityNames, double* i_dPointXYZ, int i_nSize)
	{
		return S_OK;
	}

	HRESULT Fire_Simulate_Status(long i_bStart)
	{
		return S_OK;
	}

	HRESULT Fire_Object_Selected(unsigned long i_ulObjID, PQDataType i_eObjType)
	{
		return S_OK;
	}

	HRESULT Fire_Arcball_Status(long i_bStart)
	{
		return S_OK;
	}

	HRESULT Fire_Simulate_Point_Error(DOUBLE i_dPtPositionX, DOUBLE i_dPtPositionY, DOUBLE i_dPtPositionZ, PQPathPtState i_ePtState)
	{
		return S_OK;
	}
	
	HRESULT Fire_Object_Deleted(unsigned long i_ulObjID, LPWSTR i_wObjName, PQDataType i_eObjType)
	{
		return S_OK;
	}

private:
	LONG m_lRefCount;


};