#ifndef _BINAnboto_BINAnboto_h
#define _BINAnboto_BINAnboto_h

#include <CtrlLib/CtrlLib.h>
#include <Controls4U/Controls4U.h>

using namespace Upp;

#define LAYOUTFILE <BINAnboto/BINAnboto.lay>
#include <CtrlCore/lay.h>

enum DataType    {Tchar, Tint8, Tint16, Tint32, Tint64, Tdouble32, Tdouble64, DataTypeNum};
int DataSize[] = {1,     1,     2,      4,      8,      4,         8};

class BinDataSource : public Convert {
public:
	void Init(FileInBinary &file, int64 from, int64 size, int phase, DataType type, int columns, int column) {
		this->file = &file;
		this->from = from;
		this->size = size;
		this->phase = phase;
		this->type = type;
		this->columns = columns;
		this->column = column;	
		sz = DataSize[type];
	}
	Value Format(const Value& q) const;
	
private:
	FileInBinary *file = nullptr;
	int64 from, size;
	int phase;
	DataType type;
	int columns, column;
	int sz;
};

class BINAnboto : public WithBINAnbotoLayout<TopWindow> {
public:
	BINAnboto();
	virtual ~BINAnboto();
	void Init();
	void End();

	void Jsonize(JsonIO &json);

private:
	LineEdit strings;
	ArrayCtrl int8s, int16s, int32s, int64s, double32s, double64s;
	Array<ArrayCtrl *> arrays;
	Array<BinDataSource> int8data, int16data, int32data, int64data, double32data, double64data;
	Array<Array<BinDataSource> *> arraydata;
	
	Array<Vector<int>> AOIs;
	
	void Load();
	void Update();
	void UpdateStrings();
	void UpdateNums();
	void NextAOI();
	void Search();
	void ArraySel(ArrayCtrl &array, DataType type);
	void ArraySel();
	void StringSel();
	
	void LoadSerializeJson(bool &firstTime);
	void StoreSerializeJson();
	
	FileInBinary file;
	int64 size = 0;
	String fileName;
};

#endif
