// SplineKnotAlign.cpp : Defines the exported functions for the DLL application.
//
// Copyright (c) 2011, Varun Raj Naga. This source code and accompanying binaries are governed by the BSD-2 clause license.
// See License.txt for details.

#include "stdafx.h"
//use GetWindowLongPtr/GWLP_USERDATA instead of GetWindowLong/GWL_USERDATA for compatibility with 64 bit code
//Static instance of SplineKnotAlign to prevent multiple windows
//Static Dll handle
static HMODULE hDllInstance= 0;
static ::SplineKnotAlign leSplineKnotAlign;;
//============================================================
// Class descriptor declaration

class SplineKnotAlignClassDesc
    : public ClassDesc2
{
public:
    //---------------------------------------
    // ClassDesc2 overrides 

	virtual int IsPublic() {return 1; }
	virtual void* Create(BOOL loading = FALSE) 
	{
		//return new ::SplineKnotAlign();
		return &leSplineKnotAlign;
	}
	virtual const MCHAR* ClassName() {return TEXT("SplineKnotAlign"); }
	virtual SClass_ID SuperClassID() {return UTILITY_CLASS_ID;}
	virtual Class_ID ClassID() {return Class_ID(0x3ED8531E, 0x5D42DE8F);}
	virtual const MCHAR* Category() {return TEXT("utility plugin");}
	virtual const MCHAR* GetInternalName(){ return TEXT("Spline vertex align tool");}
	virtual HINSTANCE HInstance(){ return hDllInstance; }

    //---------------------------------------
    // Returns a singleton instance of this class descriptor
    static ClassDesc2* GetClassDescInstance() { static SplineKnotAlignClassDesc desc; return &desc; }
};


SplineKnotAlign::SplineKnotAlign(): UtilityObj()
{         

}

SplineKnotAlign::~SplineKnotAlign()
{ 

}


static BOOL CALLBACK ToolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	SplineKnotAlign* leSplineAlign= (SplineKnotAlign*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
switch(Message)
{
	case WM_INITDIALOG:

		leSplineAlign = (SplineKnotAlign*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)leSplineAlign);
		SetFocus(hwnd);
	break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case IDC_BUTTON1:
				leSplineAlign->vtxAlign(0);
			break;

			case IDC_BUTTON2:
				leSplineAlign->vtxAlign(1);
			break;

			case IDC_BUTTON3:
				leSplineAlign->vtxAlign(2);
			break;

			case IDCANCEL:
				leSplineAlign->SplineIU->CloseUtility();
				DestroyWindow(hwnd);
			break;
		}
	break;

	default:
		return FALSE;
	break;
	}
	return TRUE;
}


//=========================================================================
// UtilityPlugin<UtilityObj> overrides 
void SplineKnotAlign::BeginEditParams (Interface *ip, IUtil *iu)
{
	if (this->hRollup)
	{
		::CloseWindow(this->hRollup);
		DestroyWindow(this->hRollup);
	}
	this->SplineIP= ip; this->SplineIU= iu;
	//this->hRollup= ip->AddRollupPage(hDllInstance, MAKEINTRESOURCE(IDD_DIALOG1), ToolDlgProc, "some page", (LPARAM)this, 0, ROLLUP_CAT_STANDARD);
	this->hRollup= CreateDialogParam(hDllInstance, MAKEINTRESOURCE(IDD_DIALOG1), ip->GetMAXHWnd(), ToolDlgProc, (LPARAM)this);
	//::GeomObject* leSphere= (GeomObject*)ip->CreateInstance(SClass_ID(GEOMOBJECT_CLASS_ID), Class_ID(SPHERE_CLASS_ID, 0));
	//leSphere->GetParamBlock()->SetValue(0, ip->GetTime(), 1.32f);
	//::INode* leNode= (INode*)ip->CreateObjectNode(leSphere);
	//leNode->SetName("small ball");
}

void SplineKnotAlign::EndEditParams (Interface *ip, IUtil *iu)
{
	this->SplineIP->DeleteRollupPage(this->hRollup);
}

void SplineKnotAlign::SelectionSetChanged (Interface *ip, IUtil *iu) 
{

}

void SplineKnotAlign::DeleteThis()
{
    //delete this;
}

//=========================================================================
// UtilityPlugin overrides

ClassDesc2* SplineKnotAlign::GetClassDesc()
{
    return SplineKnotAlignClassDesc::GetClassDescInstance();
}

//custom functions
void SplineKnotAlign::vtxAlign(const int& axis)
{
	::Point3 average(0,0,0);
	if(this->SplineIP->GetSelNodeCount() != 1)
		return;

	::ObjectState oState= this->SplineIP->GetSelNode(0)->EvalWorldState(SplineIP->GetTime());
	if (oState.obj->ClassID().PartA() != SPLINE3D_CLASS_ID && oState.obj->ClassID().PartA() != SPLINESHAPE_CLASS_ID)
		return;
	//Use Spline3D::GetVert(int index) and Spline3D::SetVert(int index, const Point3& value) to get and set positions. Knot coordinates different from vertex coordinates
	//SplineShape => BezierShape => Spline3D

	::SplineShape* leSpline= (SplineShape*) oState.obj;
	BitArray* leVertSel= leSpline->shape.vertSel.sel;
	
	::Spline3D** lesSpline3D= leSpline->shape.splines;
	int selVertCount= 0;

	//calculate average position of spline vertices
	//vertSel array also contains in and out vectors, however they apparently cannot be selected. Remains to be confirmed.
	//the vertSel array therefore contains (control vertices * 3) elements
	for (int i = 0; i < leSpline->shape.splineCount; ++i)
	{
		selVertCount += leVertSel[i].NumberSet();

		for (int j=0; j < (leVertSel[i].GetSize()); ++j)
			//::MessageBoxA(NULL, ToString<float>(lesSpline3D[i]->GetVert(j).x).c_str(), "Caption", MB_OK);
			if (leVertSel[i][j] == 1)
				average += lesSpline3D[i]->GetVert(j);
	}
	if (!selVertCount)
		return;

	average /= selVertCount;
	//MessageBox(NULL, ToString<int>(average.x).c_str(), NULL, MB_OK);
	//MessageBox(NULL, ToString<int>(selVertCount).c_str(), NULL, MB_OK);

	//align vertices to selected axis
	if (!::theHold.Holding())
		theHold.Begin();

	if (theHold.Holding())
	{
		theHold.Put(new UndoSpace::SplineAlignRestore(SplineIP->GetSelNode(0), SplineIP));
		SplineIP->GetSelNode(0)->SetAFlag(A_HELD);
		theHold.Accept("Spline vertex align");
	}

	for (int i=0; i < leSpline->shape.splineCount; ++i)
	{
		for (int j= 0; j< leVertSel[i].GetSize(); ++j)
			if (leVertSel[i][j])
			{
				Point3 temp= lesSpline3D[i]->GetVert(j);
				switch(axis)
				{
					case 0:
						lesSpline3D[i]->SetVert(j, Point3(average.x, temp.y, temp.z));
					break;

					case 1:
						lesSpline3D[i]->SetVert(j, Point3(temp.x, average.y, temp.z));
					break;

					case 2:
						lesSpline3D[i]->SetVert(j, Point3(temp.x, temp.y, average.z));
					break;
				}
			}
		lesSpline3D[i]->ComputeBezPoints();
		lesSpline3D[i]->InvalidateGeomCache();
	}
	leSpline->shape.InvalidateGeomCache();
	leSpline->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	leSpline->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
	SplineIP->GetSelNode(0)->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE );
	SplineIP->GetSelNode(0)->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
	
	SplineIP->ForceCompleteRedraw();
	
	return;
}

//dll entry point and exported functions
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	hDllInstance= hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

__declspec( dllexport ) const TCHAR * LibDescription() 
{ 
	return TEXT("Spline vertex align plug-in (c) November 2011"); 
} 
__declspec( dllexport ) int LibNumberClasses() 
{ 
	return 1;  
} 
__declspec( dllexport ) ClassDesc2* LibClassDesc(int i) 
{ 
    static SplineKnotAlignClassDesc classdesc; 
    return &classdesc; 
} 
__declspec( dllexport ) ULONG LibVersion() 
{ 
	return MAX_2010;
} 
__declspec( dllexport ) ULONG CanAutoDefer() 
{ 
	return 1; 
} 
__declspec( dllexport ) ULONG LibInitialize() 
{ 
    // Note: called after the DLL is loaded (i.e. DllMain is called with DLL_PROCESS_ATTACH)
    return 1;
}
__declspec( dllexport ) ULONG LibShutdown() 
{ 
    // Note: called before the DLL is unloaded (i.e. DllMain is called with DLL_PROCESS_DETACH)
    return 1;
}