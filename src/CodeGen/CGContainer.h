#ifndef _CGCTNER_H_
#define _CGCTNER_H_

using namespace llvm;
using namespace std;

enum ValueToken {
	NoneValueKind = 0,
	ExpressionListKind,
};

class ValueKind {
	vector<ValueToken> tokens;

public:

	ValueKind()
	{ }

	ValueKind(ValueToken t1)
	{
		tokens.push_back(t1);
	}

	ValueKind(ValueToken t1, ValueToken t2)
	{
		tokens.push_back(t1);
		tokens.push_back(t2);
	}

	ValueKind(ValueToken t1, ValueToken t2, ValueToken t3)
	{
		tokens.push_back(t1);
		tokens.push_back(t2);
		tokens.push_back(t3);
	}

	void addKind(ValueToken t1,
				 ValueToken t2 = NoneValueKind,
				 ValueToken t3 = NoneValueKind)
	{
		tokens.push_back(t1);
		if (t2) tokens.push_back(t2);
		if (t3) tokens.push_back(t3);
		return;
	}
	
	template <ValueToken VT>
	bool hasKind()
	{
		return find(tokens.begin(), tokens.end(), VT) != tokens.end();
	}


	virtual ~ValueKind()
	{ }
};

class CGValue {
	Value *data = NULL;
	vector<Value *> exp_list;
	ValueKind kind;

public:
	CGValue() { }

	CGValue(Value *val) :
	data(val), exp_list(vector<Value *>())
	{ }

	void pushExp(Value *val)
	{
		exp_list.push_back(val);
		return;
	}

	void addKind(ValueToken t1,
				 ValueToken t2 = NoneValueKind,
				 ValueToken t3 = NoneValueKind)
	{
		kind.addKind(t1, t2, t3);
		return;
	}

	void setValue(Value *val)
	{
		data = val;
	}

	template <ValueToken VT>
	bool hasKind()
	{
		return kind.hasKind<VT>();
	}

	inline
	operator Value *()
	{
		return data;
	}

	virtual ~CGValue()
	{
	}
};

#endif
