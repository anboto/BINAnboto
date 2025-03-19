#include "BINAnboto.h"
#include <Eigen/Eigen.h>
#include <STEM4U/Utility.h>

String GetBINAnbotoDataFolder() {
	return AFX(GetAppDataFolder(), "BINAnboto");
}

void BINAnboto::Jsonize(JsonIO &json) {
	json
		("editFile", editFile)
		("editFrom", editFrom)
		("editPhase", editPhase)
		("editColumns", editColumns)
	;
}

void BINAnboto::LoadSerializeJson(bool &firstTime) {
	editFrom <<= Null;
	editPhase <<= 0;
	editColumns <<= 1;
	
	firstTime = false;
	String folder = GetBINAnbotoDataFolder();
	if (!DirectoryCreateX(folder))
		throw Exc(Format(t_("Impossible to create folder '%s' to store configuration file"), folder));
	else {
		String fileName = AFX(folder, "config.cf");
		if (!FileExists(fileName)) {
			firstTime = true;
			return;
		} else {
			String jsonText = LoadFile(fileName);
			if (jsonText.IsEmpty())
				throw Exc(Format(t_("Configuration file '%s' is empty"), fileName));
			else {
				String ret = LoadFromJsonError(*this, jsonText);
				if (!ret.IsEmpty())
					throw Exc(ret);
			}
		}
	}
}

void BINAnboto::StoreSerializeJson() {
	String folder = GetBINAnbotoDataFolder();
	if (!DirectoryCreateX(folder))
		throw Exc(Format(t_("Impossible to create folder '%s' to store configuration file"), folder));
	String fileName = AFX(folder, "config.cf");
	if (!StoreAsJsonFile(*this, fileName, true))
		throw Exc(Format(t_("Error saving configuration file '%s'"), fileName));
}

BINAnboto::BINAnboto() {
	CtrlLayout(*this, "Binary files reader");
	
	Sizeable().Zoomable();
}

BINAnboto::~BINAnboto() {
}

void BINAnboto::Init() {
	bool firstTime;
	LoadSerializeJson(firstTime);
	
	strings.SetReadOnly();
	
	arrays << &int8s;
	arrays << &int16s;
	arrays << &int32s;
	arrays << &int64s;
	arrays << &double32s;
	arrays << &double64s;
	
	arraydata << &int8data;
	arraydata << &int16data;
	arraydata << &int32data;
	arraydata << &int64data;
	arraydata << &double32data;
	arraydata << &double64data;
	
	tab.Add(strings.SizePos(), "String");
	tab.Add(int8s.SizePos(), "Int8");
	tab.Add(int16s.SizePos(), "Int16");
	tab.Add(int32s.SizePos(), "Int32");
	tab.Add(int64s.SizePos(), "Int64");
	tab.Add(double32s.SizePos(), "Double32");
	tab.Add(double64s.SizePos(), "Double64");
	
	tab.WhenSet = [=] {ArraySel();};
	
	editFrom.WhenAction    = [=] {Update();};
	editPhase.WhenAction   = [=] {Update();};
	editColumns.WhenAction = [=] {UpdateNums();};
	butLoad.WhenAction     = [=] {Load();};
	butSearch.WhenAction   = [=] {Search();};
	butNextAOI.WhenAction  = [=] {NextAOI();};
	
	editFile.WhenChange = [=] {Load();	return true;};
	editFile.BrowseRightWidth(40).UseOpenFolder().BrowseOpenFolderWidth(10);
	
	edSearch.WhenEnter     = [=] {Search();};
	
	for (int i = 0; i < arrays.size(); ++i) {
		arrays[i]->WhenBar  = [=](Bar &menu) {ArrayCtrlWhenBar(menu, *arrays[i]);};
		arrays[i]->WhenSel  = [=] {ArraySel(*arrays[i], DataType(i+1));};
	}
	
	strings.WhenSel = [=] {StringSel();};
	
	AOIs.SetCount(DataTypeNum); 
	
	Update();
}

void BINAnboto::End() {
	StoreSerializeJson();
	arrays.Clear();
	arraydata.Clear();
}

void BINAnboto::Load() {
	fileName = ~editFile;
	
	file.Open(fileName);
	if (!file.IsOpen())
		Exclamation(t_("Impossible to load file"));
	
	size = file.GetSize();
	
	Update();
	StringSel();
}

void BINAnboto::UpdateStrings() {
	WaitCursor wait;
	
	strings.Clear();

	if (file.IsError()) {
		strings.Insert(0, "Error");
		return;
	}
	if (size == 0)
		return;
	
	int64 from = Nvl((int64)~editFrom, (int64)0);
	int phase = ~editPhase;
	
	file.Seek(from + phase);
	String str;
	str.Reserve(size - from - phase); 
	while (!file.IsEof()) {
		int c = file.Get();
		if (c >= 32)
			str.Cat(c);
		else if (c == '\n')
			str.Cat("\n");
		else if (c == '\r')
			;
		else if (c == ' ')
			str.Cat(" ");
		else if (c == '\t')
			str.Cat("    ");
		else if (c == '\t')
			str.Cat("NULL");
		else
			str.Cat("*");
	}
	strings.Set(str);
}

void BINAnboto::UpdateNums() {
	WaitCursor wait;
	
	int64 from = Nvl((int64)~editFrom, (int64)0);
	int phase = ~editPhase;
	int columns = ~editColumns;
	
	for (int i = 0; i < arraydata.size(); ++i) 
		arraydata[i]->SetCount(columns);
	
	for (int c = 0; c < columns; ++c) {
		for (int i = 0; i < arraydata.size(); ++i) 
			(*arraydata[i])[c].Init(file, from, size, phase, DataType(i+1), columns, c);
	}

	Vector<int> rows(arrays.size()), rowssc(arrays.size());
	for (int i = 0; i < arrays.size(); ++i) {
		rows[i] = arrays[i]->GetCursor();
		rowssc[i] = arrays[i]->GetCursorSc();
		arrays[i]->Reset();		
		arrays[i]->MultiSelect();	
	}
	
	for (int c = 0; c < columns; ++c) 
		for (int i = 0; i < arraydata.size(); ++i) 
			arrays[i]->AddRowNumColumn(FormatInt(c+1)).SetConvert((*arraydata[i])[c]);
	
	for (int i = 0; i < arraydata.size(); ++i) {
		arrays[i]->ClearSelection();
		int64 usable = size - phase - from*DataSize[i+1];
		int rows = usable/(DataSize[i+1]*columns);
		int diff = usable - rows*DataSize[i+1]*columns;
		if (diff > DataSize[i+1])
			rows++;
		arrays[i]->SetVirtualCount(rows);
	}
	
	for (int i = 0; i < arrays.size(); ++i) {
		if (rows[i] >= 0) {
			arrays[i]->SetCursor(rows[i]);
			arrays[i]->ScCursor(rowssc[i]);
		}
	}
}
	
void BINAnboto::Update() {
	UpdateNums();
	UpdateStrings();
}

Value BinDataSource::Format(const Value& q) const {
	int row = q;
	
	if (file->IsError())
		return "Error";
	
	int pos = phase + (row*columns + column + from)*sz;
	ASSERT(pos >= 0);
	if (pos + DataSize[type] >= size)
		return Null;
	file->Seek(pos);
	
	try {
		switch(type) {
		case Tint8:		return file->Read<int8>();
		case Tint16:	return file->Read<int16>();
		case Tint32:	return (int64)file->Read<int32>();
		case Tint64:	return file->Read<int64>();
		case Tdouble32:	return file->Read<float>();
		case Tdouble64:	return file->Read<double>();
		default:		return Null;
		};
	} catch(...) {
		return Null;
	}
}

template <typename T>
struct tiny {bool operator()(const T& lhs, const T& rhs) const {return abs(lhs) < abs(rhs);}};

template <class Range>
int FindTiny(const Range& r) {return FindBest(r, tiny<ValueTypeOf<Range>>());}

template <class Range>
const ValueTypeOf<Range>& Tiny(const Range& r) {return r[FindTiny(r)];}

bool IsAOI(FileInBinary &file, int64 size, int64 pos, DataType type, int len) {
	if ((size - pos)/DataSize[type] < len)
		return false;
	
	if (type == Tint8 || type == Tint16)
		return false;
		
	Vector<double> data(len);
	bool thereAreZeroes = false;
	file.Seek(pos);
	for (int i = 0; i < len; ++i) {
		switch(type) {
		case Tint32:	data[i] = file.Read<int32>();	break;
		case Tint64:	data[i] = file.Read<int64>();	break;
		case Tdouble32:	data[i] = file.Read<float>();	break;
		case Tdouble64:	data[i] = file.Read<double>();	break;
		default:		return false;
		};
		if (data[i] == 0)
			thereAreZeroes = true;
	}
	if (!IsNum(data))
		return false;
	double mx = Max(data);
	double mn = Min(data);
	
	double range = mx - mn;
	double big = max(abs(mx), abs(mn));
	double tiny = Tiny(data);
	
	if (type == Tdouble64) {
		if (thereAreZeroes || tiny < 1e-40 || big > 1e20 || range < 1e-20 || range > 1e30)
			return false;
	} else if (type == Tdouble32) {
		if (thereAreZeroes || tiny < 1e-40 || big > 1e20 || range < 1e-20 || range > 1e30)
			return false;
	} else if (type == Tint64) {
		if (thereAreZeroes || big > 1e10 || range > 1e20)
			return false;
	} else if (type == Tint32) {
		if (thereAreZeroes || big > 1e10 || range > 1e20)
			return false;
	}
	return true;
}

bool IsAOI(const String &str) {
	for (int i = 0; i < str.GetCount(); ++i) {
		int c = str[i];
		if (!(IsDigit(c) || IsLetter(c) || c == '_' || c == '.' || c == ','))
			return false;
	}
	return true;
}

void ScanAOIs(Array<Vector<int>> &AOIs, FileInBinary &file, int64 size, int from, int phase, DataType type) {
	int windowWidth = 10;
	int64 num = (size - phase)/DataSize[type] - from - windowWidth;
	if (num < 10)
		return;
	
	Vector<int64> toTest;
	if (num < 1000) {
		toTest.SetCount(num);
		for (int i = 0; i < num; ++i)
			toTest[i] = phase + (from + i)*DataSize[type];
	} else {
		toTest.SetCount(1000);
		for (int i = 0; i < 1000; ++i)
			toTest[i] = phase + int(num*i/1000.);
	}
	Vector<double> isAOI(toTest.size());
	for (int i = 0; i < toTest.size(); ++i)
		isAOI[i] = IsAOI(file, size, toTest[i], type, 10);	

	PromptOK(Format(t_("Found %d AOI"), Sum(isAOI)));
	
	//	
}

void BINAnboto::Search() {
	String sfind = Trim(~edSearch);
	if (sfind.IsEmpty()) {
		Exclamation(t_("No data to search"));
		return;
	}
	
	int phase = ~editPhase;
	int64 from = Nvl((int64)~editFrom, (int64)0);
		
	DataType type = (DataType)tab.Get();
	if (type == Tchar) {
		int64 cursor = strings.GetCursor64();	
		if (cursor < 0)
			cursor = 0;
		else
			cursor++;
		
		int len = sfind.GetCount();
		int64 stringsSize = strings.GetLength64();
		for (int64 searchPos = 0; searchPos < stringsSize; searchPos++) {
			String str = strings.Get(searchPos, len);
			if (strings.Get(searchPos, len) == sfind) {
				strings.SetFocus();
				strings.SetCursor(searchPos);
				//strings.SetSelection(searchPos, len);
				return;
			}
		}
	} else {
		int columns = ~editColumns;
		int row = arrays[type-1]->GetCursor();
		if (row < 0)
			row = 0;
		else
			row++;	// Search from next row
		if (row >= arrays[type-1]->GetCount()) {
			Exclamation(t_("Data not found"));
			return;
		}
		int64 actualByte = phase + (row*columns + from)*DataSize[type];
		
		file.Seek(actualByte);
		for (int64 searchByte = actualByte; searchByte+DataSize[type] < size; searchByte += DataSize[type]) {
			String str;
			switch(type) {
			case Tint8:		str << file.Read<int8>();			break;
			case Tint16:	str << file.Read<int16>();			break;
			case Tint32:	str << (int64)file.Read<int32>();	break;
			case Tint64:	str << file.Read<int64>();			break;
			case Tdouble32:	str << file.Read<float>();			break;
			case Tdouble64:	str << file.Read<double>();			break;
			default:		NEVER();	return;
			}
			if (str.Find(sfind) >= 0) {
				ArrayCtrl &array = *(arrays[type-1]);
				int row = (searchByte - phase - from*DataSize[type])/(columns*DataSize[type]);
				int sc = array.GetCursorSc();
				array.SetFocus();
				array.SetCursor(row);
				array.ScCursor(sc);
				return;
			}
		}
	}
	Exclamation(t_("Data not found"));
}

void BINAnboto::NextAOI() {
	DataType type = (DataType)tab.Get();
	if (type == Tchar) {
		int64 cursor = strings.GetCursor64();	
		if (cursor < 0)
			cursor = 0;
		else
			cursor++;
		
		int len = 10;
		int64 stringsSize = strings.GetLength64();
		for (int64 searchPos = cursor; searchPos + 10 < stringsSize; searchPos++) {
			String str = strings.Get(searchPos, len);
			if (IsAOI(strings.Get(searchPos, len))) {
				strings.SetFocus();
				strings.SetCursor(searchPos);
				//strings.SetSelection(searchPos, len);
				return;
			}
		}		
	} else {
		int columns = ~editColumns;
		int phase = ~editPhase;
		int64 from = Nvl((int64)~editFrom, (int64)0);
		int row = arrays[type-1]->GetCursor();
		if (row < 0)
			row = 0;
		row++;	// Search from next row
		if (row >= arrays[type-1]->GetCount()) {
			Exclamation(t_("Data not found"));
			return;
		}
		int64 actualByte = phase + (row*columns + from)*DataSize[type];
		
		for (int64 searchByte = actualByte; searchByte+DataSize[type]*10 < size; searchByte += DataSize[type]) {	// Data by data instead of row by row
			if (IsAOI(file, size, searchByte, type, 10)) {
				int row = (searchByte - phase - from*DataSize[type])/(columns*DataSize[type]);
				int sc = arrays[type-1]->GetCursorSc();
				arrays[type-1]->SetCursor(row);
				arrays[type-1]->ScCursor(sc);
				return;
			}
		}
	}
	Exclamation(t_("No AOI (Area Of Interest) is found"));
}

void BINAnboto::ArraySel(ArrayCtrl &array, DataType type) {
	int row = array.GetCursor();
	if (row < 0) {
		edRow <<= Null; 
		edItem <<= Null;
	} else {
		int64 from = Nvl((int64)~editFrom, (int64)0);
		int phase = ~editPhase;
		int columns = ~editColumns;
	
		edRow <<= row;	
		int64 actualByte = phase + (row*columns + from)*DataSize[type];
		edItem <<= actualByte;
	}
}

void BINAnboto::ArraySel() {
	DataType type = (DataType)tab.Get();
	if (type > 0)
		ArraySel((*arrays[type-1]), type);
}

void BINAnboto::StringSel() {
	if (!file)
		return;
	
	int64 from = Nvl((int64)~editFrom, (int64)0);
	int phase = ~editPhase;
		
	int64 pos = strings.GetCursor64();
	edItem <<= pos + phase + from;
	
	edRow <<= strings.GetLinePos64(pos);
}
	
GUI_APP_MAIN
{
	String errorStr;
	try {
		BINAnboto main;
		
		main.Init();
		main.Run();
		main.End();
	} catch (Exc e) {
		errorStr = e;
	} catch(const char *cad) {
		errorStr = cad;
	} catch(const std::string &e) {
		errorStr = e.c_str();	
	} catch (const std::exception &e) {
		errorStr = e.what();
	} catch(...) {
		errorStr = t_("Unknown error");
	}
	if (!errorStr.IsEmpty()) {
		Exclamation(Format(t_("Problem found: %s"), errorStr));
		SetExitCode(-1);
	}
}
