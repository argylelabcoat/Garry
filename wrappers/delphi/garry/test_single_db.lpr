program test_single_db;
{$MODE DELPHI}
{$C+}
uses
  SysUtils, garry, garry_types, garry_codec;

function BytesOf(const Vals: array of Byte): TGarryBytes;
var I: Integer;
begin
  SetLength(Result, Length(Vals));
  for I := 0 to High(Vals) do Result[I] := Vals[I];
end;

var
  DB: TGarryDatabase;
  Key, Val, BinKey, BinVal, Value: TGarryBytes;
  Txn, Count, I: Integer;
  Cursor: TGarryCursor;
  St: TGarryStatus;
begin
  DeleteFile('test_single.db');
  DB := TGarryDatabase.Create('test_single.db', True);

  { String keys }
  DB.SetKeyValue('hello', BytesOf([$48]));
  DB.SetKeyValue('test_key', BytesOf([$01, $02, $03]));
  DB.Delete('hello');
  DB.Delete('test_key');

  { Fresh for count }
  DB.Free;
  DeleteFile('test_single.db');
  DB := TGarryDatabase.Create('test_single.db', True);
  DB.SetKeyValue('a', BytesOf([$01]));
  DB.SetKeyValue('b', BytesOf([$02]));
  DB.SetKeyValue('c', BytesOf([$03]));

  { Cursor }
  DB.SetKeyValue(EncodeKey(['prefix', 'a']), BytesOf([$01]));
  DB.SetKeyValue(EncodeKey(['prefix', 'b']), BytesOf([$02]));
  DB.SetKeyValue(EncodeKey(['prefix', 'c']), BytesOf([$03]));
  Cursor := DB.OpenCursor(EncodeKey(['prefix']));
  Count := 0;
  while Cursor.Next(Key, Val) do Inc(Count);
  Cursor.Free;
  Assert(Count = 3, 'cursor prefix count');

  Cursor := DB.OpenCursorAll;
  Count := 0;
  while Cursor.Next(Key, Val) do Inc(Count);
  Cursor.Free;
  WriteLn('cursor all count=', Count);

  { Binary key }
  BinKey := BytesOf([$DE, $AD, $BE, $EF]);
  BinVal := BytesOf([$12, $34, $56, $78, $9A]);
  DB.SetKeyValue(BinKey, BinVal);
  Assert(DB.Exists(BinKey), 'binary key exists');
  Value := DB.Get(BinKey);
  Assert(Length(Value) = 5, 'binary value len');
  Assert(DB.Delete(BinKey), 'delete binary key');
  Assert(not DB.Exists(BinKey), 'binary key gone after delete');

  { Large value }
  SetLength(BinVal, 256);
  for I := 0 to High(BinVal) do BinVal[I] := Byte(I mod 256);
  DB.SetKeyValue('large_key', BinVal);
  Value := DB.Get('large_key');
  Assert(Length(Value) = 256, 'large value len');

  { Nav }
  DB.SetKeyValue(EncodeKey(['nav', '001']), BytesOf([$01]));
  DB.SetKeyValue(EncodeKey(['nav', '002']), BytesOf([$02]));
  DB.SetKeyValue(EncodeKey(['nav', '003']), BytesOf([$03]));
  Value := DB.First;
  Assert(Length(Value) > 0, 'first');
  Value := DB.Last;
  Assert(Length(Value) > 0, 'last');
  Value := DB.NextKey(EncodeKey(['nav', '001']));
  Assert(Length(Value) > 0, 'nextkey');
  Value := DB.PrevKey(EncodeKey(['nav', '003']));
  Assert(Length(Value) > 0, 'prevkey');

  WriteLn('About to free...');
  Flush(Output);
  DB.Free;
  WriteLn('Free OK');
  DeleteFile('test_single.db');
  WriteLn('All OK');
end.
