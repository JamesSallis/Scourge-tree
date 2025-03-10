/*	see copyright notice in squirrel.h */
#ifndef _SQCLASS_H_
#define _SQCLASS_H_

struct SQInstance;

struct SQClassMemeber {
	SQClassMemeber(){}
	SQClassMemeber(const SQClassMemeber &o) {
		val = o.val;
		attrs = o.attrs;
	}
	SQObjectPtr val;
	SQObjectPtr attrs;
};

typedef sqvector<SQClassMemeber> SQClassMemeberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((SQInteger)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((SQInteger)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct SQClass : public CHAINABLE_OBJ
{
	SQClass(SQSharedState *ss,SQClass *base);
public:
	static SQClass* Create(SQSharedState *ss,SQClass *base) {
		SQClass *newclass = (SQClass *)SQ_MALLOC(sizeof(SQClass));
		new (newclass) SQClass(ss, base);
		return newclass;
	}
	virtual ~SQClass();
	bool NewSlot(const SQObjectPtr &key,const SQObjectPtr &val);
	bool Get(const SQObjectPtr &key,SQObjectPtr &val) {
		if(_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr &o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool SetAttributes(const SQObjectPtr &key,const SQObjectPtr &val);
	bool GetAttributes(const SQObjectPtr &key,SQObjectPtr &outval);
	void Lock() { _locked = true; if(_base) _base->Lock(); }
	void Release() { sq_delete(this, SQClass);	}
	void Finalize();
	void Mark(SQCollectable ** );
	SQInteger Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
	SQInstance *CreateInstance();
	SQTable *_members;
	//SQTable *_properties;
	SQClass *_base;
	SQClassMemeberVec _defaultvalues;
	SQClassMemeberVec _methods;
	SQObjectPtrVec _metamethods;
	SQObjectPtr _attributes;
	SQUserPointer _typetag;
	bool _locked;
};

#define calcinstancesize(_theclass_) \
	(sizeof(SQInstance)+(sizeof(SQObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0)))
struct SQInstance : public SQDelegable 
{
	void Init(SQSharedState *ss);
	SQInstance(SQSharedState *ss, SQClass *c, SQInteger memsize);
	SQInstance(SQSharedState *ss, SQInstance *c, SQInteger memsize);
public:
	static SQInstance* Create(SQSharedState *ss,SQClass *theclass) {
		
		SQInteger size = calcinstancesize(theclass);
		SQInstance *newinst = (SQInstance *)SQ_MALLOC(size);
		new (newinst) SQInstance(ss, theclass,size);
		return newinst;
	}
	SQInstance *Clone(SQSharedState *ss)
	{
		SQInteger size = calcinstancesize(_class);
		SQInstance *newinst = (SQInstance *)SQ_MALLOC(size);
		new (newinst) SQInstance(ss, this,size);
		return newinst;
	}
	virtual ~SQInstance();
	bool Get(const SQObjectPtr &key,SQObjectPtr &val)  {
		if(_class->_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr &o = _values[_member_idx(val)];
				val = _realval(o);
			}
			else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool Set(const SQObjectPtr &key,const SQObjectPtr &val) {
		SQObjectPtr idx;
		if(_class->_members->Get(key,idx) && _isfield(idx)) {
            _values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}
	void Release() { 
		if (_hook) { _hook(_userpointer,0);}
		SQInteger size = _memsize;
		this->~SQInstance();
		SQ_FREE(this, size);
	}
	void Finalize();
	void Mark(SQCollectable ** );
	bool InstanceOf(SQClass *trg);
	bool GetMetaMethod(SQMetaMethod mm,SQObjectPtr &res);

	SQClass *_class;
	SQUserPointer _userpointer;
	SQRELEASEHOOK _hook;
	SQUnsignedInteger _nvalues;
	SQInteger _memsize;
	SQObjectPtr _values[1];
};

#endif //_SQCLASS_H_
