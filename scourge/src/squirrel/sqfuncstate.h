/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"

struct SQFuncState
{
	SQFuncState(SQSharedState *ss,SQFuncState *parent,CompilerErrorFunc efunc,void *ed);
	~SQFuncState();
#ifdef _DEBUG_DUMP
	void Dump(SQFunctionProto *func);
#endif
	void Error(const SQChar *err);
	SQFuncState *PushChildState(SQSharedState *ss);
	void PopChildState();
	void AddInstruction(SQOpcode _op,SQInteger arg0=0,SQInteger arg1=0,SQInteger arg2=0,SQInteger arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
	void AddInstruction(SQInstruction &i);
	void SetIntructionParams(SQInteger pos,SQInteger arg0,SQInteger arg1,SQInteger arg2=0,SQInteger arg3=0);
	void SetIntructionParam(SQInteger pos,SQInteger arg,SQInteger val);
	SQInstruction &GetInstruction(SQInteger pos){return _instructions[pos];}
	void PopInstructions(SQInteger size){for(SQInteger i=0;i<size;i++)_instructions.pop_back();}
	void SetStackSize(SQInteger n);
	void SnoozeOpt(){_optimization=false;}
	SQInteger GetCurrentPos(){return _instructions.size()-1;}
	//SQInteger GetStringConstant(const SQChar *cons);
	SQInteger GetNumericConstant(const SQInteger cons);
	SQInteger GetNumericConstant(const SQFloat cons);
	SQInteger PushLocalVariable(const SQObject &name);
	void AddParameter(const SQObject &name);
	void AddOuterValue(const SQObject &name);
	SQInteger GetLocalVariable(const SQObject &name);
	SQInteger GetOuterVariable(const SQObject &name);
	SQInteger GenerateCode();
	SQInteger GetStackSize();
	SQInteger CalcStackFrameSize();
	void AddLineInfos(SQInteger line,bool lineop,bool force=false);
	SQFunctionProto *BuildProto();
	SQInteger AllocStackPos();
	SQInteger PushTarget(SQInteger n=-1);
	SQInteger PopTarget();
	SQInteger TopTarget();
	SQInteger GetUpTarget(SQInteger n);
	bool IsLocal(SQUnsignedInteger stkpos);
	SQObject CreateString(const SQChar *s,SQInteger len = -1);
	SQInteger _returnexp;
	SQLocalVarInfoVec _vlocals;
	SQIntVec _targetstack;
	SQInteger _stacksize;
	bool _varparams;
	bool _bgenerator;
	SQIntVec _unresolvedbreaks;
	SQIntVec _unresolvedcontinues;
	SQObjectPtrVec _functions;
	SQObjectPtrVec _parameters;
	SQOuterVarVec _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec _localvarinfos;
	SQObjectPtr _literals;
	SQObjectPtr _strings;
	SQObjectPtr _name;
	SQObjectPtr _sourcename;
	SQInteger _nliterals;
	SQLineInfoVec _lineinfos;
	SQFuncState *_parent;
	SQIntVec _breaktargets; //contains number of nested exception traps
	SQIntVec _continuetargets;
	SQInteger _lastline;
	SQInteger _traps;
	bool _optimization;
	SQSharedState *_sharedstate;
	sqvector<SQFuncState*> _childstates;
	SQInteger GetConstant(const SQObject &cons);
private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};


#endif //_SQFUNCSTATE_H_

