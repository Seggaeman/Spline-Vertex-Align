// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

// Copyright (c) 2011, Varun Raj Naga. This source code and accompanying binaries are governed by the BSD-2 clause license.
// See License.txt for details.

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
// TODO: reference additional headers your program requires here

#include <iostream>
#include <sstream>
#include <vector>
#include <utilapi.h>
#include <iparamb2.h>
#include <point3.h>
#include <matrix3.h>
#include <triobj.h>
#include <dummy.h>
#include <stdmat.h>
#include <simpobj.h>
#include <control.h>
#include <istdplug.h>
#include <splshape.h>
#include "resource.h"


#define MAX_2010 ((MAX_RELEASE_R12<<16)+(MAX_API_NUM_R120<<8)+MAX_SDK_REV)


//============================================================
// The plug-in definition

class SplineKnotAlign 
    : public UtilityObj
{
    //============================================================
    // Fields

    // Use member fields only when data is not managed by the parameter block
    // or the reference manager. 

private:
	template<typename var> inline std::string ToString(var input)
	{
		std::stringstream temp;
		temp << input;
		return temp.str();
	}

public:
   	IUtil* SplineIU;
	Interface* SplineIP;
	HWND hRollup;
    //============================================================
    // Constructor/destructor
	SplineKnotAlign();
	~SplineKnotAlign();
	virtual void BeginEditParams(Interface*, IUtil* );
	virtual void EndEditParams(Interface*, IUtil* );
	virtual void SelectionSetChanged(Interface*, IUtil*);
	virtual void DeleteThis();
	virtual ClassDesc2* GetClassDesc();
	
	//Custom functions
	virtual void vtxAlign(const int& axis);
	template <typename var> var getObjectFromNode(INode* node, BOOL &deleteIt, ULONG const& ClassIDA)
	{
		deleteIt = FALSE;
		//Get current time from UI
		TimeValue t = SplineIP->GetTime();
		//Evaluate object at current time
		Object *obj = node->GetObjectRef();
		//Can we convert it in a TriObject ?
		if (obj && obj->CanConvertToType(Class_ID(ClassIDA, 0)))
		{
			//Let’s convert the object in a TriObject
			var theObject = (var)obj->ConvertToType(t,Class_ID(ClassIDA, 0));
			// Note that the TriObject should only be deleted
			// if the pointer to it is not equal to the object
			// pointer that called ConvertToType()
			if ((var)obj!= theObject)
				deleteIt = TRUE;
			return theObject;
		}
		else
			return NULL;
	}
};

//======================================================================

namespace UndoSpace {
class SplineAlignRestore: public ::RestoreObj
{
private:
	std::vector<std::vector<Point3> > undoVertices;
	std::vector<std::vector<Point3> > redoVertices;
	::INode* curSplineNode;
	::SplineShape* curSplineShape;
	Interface* iInterface;

public:
	
	SplineAlignRestore(INode* inpSpline, Interface* ii)
	{
		this->curSplineNode= inpSpline;
		this->iInterface= ii;
		::ObjectState oState= curSplineNode->EvalWorldState(ii->GetTime());
		curSplineShape= (SplineShape*) oState.obj;
		undoVertices.resize(this->curSplineShape->shape.SplineCount());
		redoVertices.resize(this->curSplineShape->shape.SplineCount());
		for (int i= 0; i < undoVertices.size(); ++i)
		{
			for (int j= 0; j < curSplineShape->shape.splines[i]->Verts(); ++j)
				undoVertices[i].push_back(curSplineShape->shape.splines[i]->GetVert(j));
		}
	}

	void Restore(int isUndo)
	{
		if (isUndo)
			for (int i= 0; i < undoVertices.size(); ++i)
			{
				for (int j= 0; j < curSplineShape->shape.splines[i]->Verts(); ++j)
				{
					redoVertices[i].push_back(curSplineShape->shape.splines[i]->GetVert(j));
					curSplineShape->shape.splines[i]->SetVert(j, undoVertices[i][j]);
				}
				curSplineShape->shape.splines[i]->ComputeBezPoints();
				curSplineShape->shape.splines[i]->InvalidateGeomCache();
			}
		else
			for (int i= 0; i < undoVertices.size(); ++i)
			{
				for (int j= 0; j < curSplineShape->shape.splines[i]->Verts(); ++j)
					curSplineShape->shape.splines[i]->SetVert(j, undoVertices[i][j]);

				curSplineShape->shape.splines[i]->ComputeBezPoints();
				curSplineShape->shape.splines[i]->InvalidateGeomCache();
			}

		curSplineShape->shape.InvalidateGeomCache();
		curSplineShape->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		curSplineShape->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
		curSplineNode->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE );
		curSplineNode->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
	
		this->iInterface->ForceCompleteRedraw();

	}

	void Redo()
	{
		for (int i= 0; i < undoVertices.size(); ++i)
		{
			for (int j= 0; j < curSplineShape->shape.splines[i]->Verts(); ++j)
				curSplineShape->shape.splines[i]->SetVert(j, redoVertices[i][j]);

			curSplineShape->shape.splines[i]->ComputeBezPoints();
			curSplineShape->shape.splines[i]->InvalidateGeomCache();
		}

		curSplineShape->shape.InvalidateGeomCache();
		curSplineShape->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		curSplineShape->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
		curSplineNode->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE );
		curSplineNode->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED );
	
		this->iInterface->ForceCompleteRedraw();
	}

	int Size()
	{
		return 2*sizeof(std::vector<std::vector<Point3>>) + 3*sizeof(INode*);	//pointers of standard size: 4 byte on 32 bit and 8 on 64 bit
	}

	void EndHold()
	{
		this->curSplineNode->ClearAFlag(A_HELD);
	}

	virtual TSTR Description()
	{
		return TSTR(TEXT("Spline vertex align"));
	}
};

}