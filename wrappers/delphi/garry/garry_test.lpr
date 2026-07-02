program garry_test;

{$MODE DELPHI}
{$C+}

uses
  SysUtils, garry, garry_types, garry_codec;

type
  TLevel10 = record
    DeepValue: AnsiString;
  end;

  TLevel9 = record
    Child: TLevel10;
  end;

  TLevel8 = record
    Child: TLevel9;
  end;

  TLevel7 = record
    Child: TLevel8;
  end;

  TLevel6 = record
    Child: TLevel7;
  end;

  TLevel5 = record
    Child: TLevel6;
  end;

  TLevel4 = record
    Child: TLevel5;
  end;

  TLevel3 = record
    Child: TLevel4;
  end;

  TLevel2 = record
    Child: TLevel3;
  end;

  TLevel1 = record
    Child: TLevel2;
  end;

  TDeepRoot = record
    Child: TLevel1;
  end;

function BytesOf(const Vals: array of Byte): TGarryBytes;
var
  I: Integer;
begin
  SetLength(Result, Length(Vals));
  for I := 0 to High(Vals) do
    Result[I] := Vals[I];
end;

function EncodeDeepString(const S: AnsiString): TGarryBytes;
var
  UB: TGarryBytes;
  I: Integer;
begin
  SetLength(UB, Length(S));
  for I := 1 to Length(S) do
    UB[I - 1] := Byte(S[I]);
  Result := EncodeValueBytes(UB);
end;

function DecodeDeepString(const Data: TGarryBytes): AnsiString;
var
  UB: TGarryBytes;
  I: Integer;
begin
  UB := DecodeBytes(Data);
  SetLength(Result, Length(UB));
  for I := 0 to Length(UB) - 1 do
    Result[I + 1] := AnsiChar(UB[I]);
end;

function SerializeDeepRoot(const Root: TDeepRoot): TGarryBytes;
var
  L10, L9, L8, L7, L6, L5, L4, L3, L2, L1, RootEnc: TGarryBytes;
begin
  L10 := EncodeDeepString(Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue);
  L9 := EncodeValueBytes(L10);
  L8 := EncodeValueBytes(L9);
  L7 := EncodeValueBytes(L8);
  L6 := EncodeValueBytes(L7);
  L5 := EncodeValueBytes(L6);
  L4 := EncodeValueBytes(L5);
  L3 := EncodeValueBytes(L4);
  L2 := EncodeValueBytes(L3);
  L1 := EncodeValueBytes(L2);
  RootEnc := EncodeValueBytes(L1);
  Result := RootEnc;
end;

function DeserializeDeepRoot(const Data: TGarryBytes): TDeepRoot;
var
  L1, L2, L3, L4, L5, L6, L7, L8, L9, L10: TGarryBytes;
begin
  L1 := DecodeBytes(Data);
  L2 := DecodeBytes(L1);
  L3 := DecodeBytes(L2);
  L4 := DecodeBytes(L3);
  L5 := DecodeBytes(L4);
  L6 := DecodeBytes(L5);
  L7 := DecodeBytes(L6);
  L8 := DecodeBytes(L7);
  L9 := DecodeBytes(L8);
  L10 := DecodeBytes(L9);
  Result.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := DecodeDeepString(L10);
end;

procedure TestDeepNesting(DB: TGarryDatabase);
var
  Root, Recovered: TDeepRoot;
  Encoded, Value: TGarryBytes;
  I: Integer;
begin
  WriteLn('Test 18: Deep nesting (10 levels)...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := 'found it';
  Encoded := SerializeDeepRoot(Root);
  Recovered := DeserializeDeepRoot(Encoded);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue = 'found it',
    'Deep value mismatch');
  WriteLn('  OK');

  WriteLn('Test 19: Deep nesting with varied values...');
  for I := 1 to 5 do
  begin
    FillChar(Root, SizeOf(Root), 0);
    Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue :=
      AnsiString('level_10_value_' + IntToStr(I));
    Encoded := SerializeDeepRoot(Root);
    Recovered := DeserializeDeepRoot(Encoded);
    Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue =
      AnsiString('level_10_value_' + IntToStr(I)),
      'Deep value mismatch at iteration ' + IntToStr(I));
  end;
  WriteLn('  OK');

  WriteLn('Test 20: Deep nesting round-trip via database...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := 'persist_me';
  Encoded := SerializeDeepRoot(Root);
  DB.SetKeyValue('deep/nesting/test', Encoded);
  Value := DB.Get('deep/nesting/test');
  Recovered := DeserializeDeepRoot(Value);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue = 'persist_me',
    'Deep value round-trip mismatch');
  DB.Delete('deep/nesting/test');
  WriteLn('  OK');

  WriteLn('Test 21: Deep nesting empty string...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := '';
  Encoded := SerializeDeepRoot(Root);
  Recovered := DeserializeDeepRoot(Encoded);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue = '',
    'Empty deep value mismatch');
  WriteLn('  OK');

  WriteLn('Test 22: Deep nesting long string...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue :=
    'AAAAAAAAAA_BBBBBBBBBB_CCCCCCCCCC_DDDDDDDDDD_EEEEEEEEEE';
  Encoded := SerializeDeepRoot(Root);
  Recovered := DeserializeDeepRoot(Encoded);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue =
    'AAAAAAAAAA_BBBBBBBBBB_CCCCCCCCCC_DDDDDDDDDD_EEEEEEEEEE',
    'Long deep value mismatch');
  WriteLn('  OK');
end;

var
  DB: TGarryDatabase;
  Value, BinKey, BinVal: TGarryBytes;
  Encoded: TGarryBytes;
  Parts: array of AnsiString;
  Success: Boolean;
  Txn: Integer;
  Cursor: TGarryCursor;
  Key, Val: TGarryBytes;
  Count, I: Integer;
  St: TGarryStatus;
begin
  try
    DeleteFile('test_fpc.db');

    WriteLn('Test 1: Create database...');
    DB := TGarryDatabase.Create('test_fpc.db', True);
    WriteLn('  OK');

    WriteLn('Test 2: Set and get (string key)...');
    DB.SetKeyValue('hello', BytesOf([$48, $65, $6C, $6C, $6F]));
    Value := DB.Get('hello');
    Assert(Length(Value) = 5, 'Expected 5 bytes');
    Assert(Value[0] = $48, 'Expected H');
    Assert(Value[4] = $6F, 'Expected o');
    WriteLn('  OK');

    WriteLn('Test 3: Delete (string key)...');
    Success := DB.Delete('hello');
    Assert(Success, 'Delete should return true');
    Assert(not DB.Exists('hello'), 'Key should not exist after delete');
    WriteLn('  OK');

    WriteLn('Test 4: Exists (string key)...');
    DB.SetKeyValue('test_key', BytesOf([$01, $02, $03]));
    Assert(DB.Exists('test_key'), 'Key should exist');
    DB.Delete('test_key');
    Assert(not DB.Exists('test_key'), 'Key should not exist after delete');
    WriteLn('  OK');

    WriteLn('Test 5: Count...');
    DB.SetKeyValue('a', BytesOf([$01]));
    DB.SetKeyValue('b', BytesOf([$02]));
    DB.SetKeyValue('c', BytesOf([$03]));
    Count := DB.Count;
    Assert(Count >= 3, 'Expected at least 3 keys');
    WriteLn('  OK');

    WriteLn('Test 6: Transaction rollback...');
    Txn := DB.BeginTxn;
    Key := BytesOf([$72, $62]);
    Val := BytesOf([$AA, $BB]);
    St := garry_set(DB.Handle, Txn, @Key[0], Length(Key), @Val[0], Length(Val));
    Assert(St = GarryOK, 'garry_set should succeed');
    DB.Rollback(Txn);
    Assert(not DB.Exists(Key), 'Key should not exist after rollback');
    WriteLn('  OK');

    WriteLn('Test 7: Transaction commit...');
    Txn := DB.BeginTxn;
    Key := BytesOf([$63, $6D]);
    Val := BytesOf([$CC]);
    St := garry_set(DB.Handle, Txn, @Key[0], Length(Key), @Val[0], Length(Val));
    Assert(St = GarryOK, 'garry_set should succeed');
    DB.Commit(Txn);
    Assert(DB.Exists(Key), 'Key should exist after commit');
    Value := DB.Get(Key);
    Assert(Length(Value) = 1, 'Key should have value after commit');
    Assert(Value[0] = $CC, 'Expected $CC');
    WriteLn('  OK');

    WriteLn('Test 8: Cursor iteration (prefix)...');
    DB.SetKeyValue(EncodeKey(['prefix', 'a']), BytesOf([$01]));
    DB.SetKeyValue(EncodeKey(['prefix', 'b']), BytesOf([$02]));
    DB.SetKeyValue(EncodeKey(['prefix', 'c']), BytesOf([$03]));
    Cursor := DB.OpenCursor(EncodeKey(['prefix']));
    Count := 0;
    while Cursor.Next(Key, Val) do
      Inc(Count);
    Cursor.Free;
    Assert(Count = 3, 'Expected 3 keys with prefix');
    WriteLn('  OK');

    WriteLn('Test 9: Cursor NextKey...');
    Cursor := DB.OpenCursor(EncodeKey(['prefix']));
    Count := 0;
    while Cursor.NextKey(Key) do
      Inc(Count);
    Cursor.Free;
    Assert(Count = 3, 'Expected 3 keys via NextKey');
    WriteLn('  OK');

    WriteLn('Test 10: Cursor via OpenCursorAll...');
    Cursor := DB.OpenCursorAll;
    Count := 0;
    while Cursor.Next(Key, Val) do
      Inc(Count);
    Cursor.Free;
    Assert(Count >= 5, 'Expected at least 5 keys total');
    WriteLn('  OK');

    WriteLn('Test 11: Binary key/value (TGarryBytes overload)...');
    BinKey := BytesOf([$DE, $AD, $BE, $EF]);
    BinVal := BytesOf([$12, $34, $56, $78, $9A]);
    DB.SetKeyValue(BinKey, BinVal);
    Assert(DB.Exists(BinKey), 'Binary key should exist');
    Value := DB.Get(BinKey);
    Assert(Length(Value) = 5, 'Expected 5 bytes');
    Assert(Value[0] = $12, 'Expected $12');
    Assert(Value[4] = $9A, 'Expected $9A');
    Assert(DB.Delete(BinKey), 'Delete binary key should succeed');
    Assert(not DB.Exists(BinKey), 'Binary key should not exist after delete');
    WriteLn('  OK');

    WriteLn('Test 12: Empty value (zero-length)...');
    DB.SetKeyValue('empty_val', BytesOf([]));
    Assert(DB.Exists('empty_val'), 'Empty value key should exist');
    Value := DB.Get('empty_val');
    Assert(Length(Value) = 0, 'Expected 0 bytes for empty value');
    DB.Delete('empty_val');
    WriteLn('  OK');

    WriteLn('Test 13: Get non-existent key...');
    Value := DB.Get('does_not_exist');
    Assert(Length(Value) = 0, 'Expected 0 bytes for missing key');
    WriteLn('  OK');

    WriteLn('Test 14: Delete non-existent key...');
    Success := DB.Delete('also_missing');
    Assert(not Success, 'Delete of missing key should return false');
    WriteLn('  OK');

    WriteLn('Test 15: NextKey / PrevKey...');
    DB.SetKeyValue(EncodeKey(['nav', '001']), BytesOf([$01]));
    DB.SetKeyValue(EncodeKey(['nav', '002']), BytesOf([$02]));
    DB.SetKeyValue(EncodeKey(['nav', '003']), BytesOf([$03]));
    Value := DB.First;
    Assert(Length(Value) > 0, 'First should return a key');
    Value := DB.Last;
    Assert(Length(Value) > 0, 'Last should return a key');
    Value := DB.NextKey(EncodeKey(['nav', '001']));
    Assert(Length(Value) > 0, 'NextKey should return a key');
    Value := DB.PrevKey(EncodeKey(['nav', '003']));
    Assert(Length(Value) > 0, 'PrevKey should return a key');
    WriteLn('  OK');

    WriteLn('Test 16: Large value...');
    SetLength(BinVal, 4096);
    for I := 0 to High(BinVal) do
      BinVal[I] := Byte(I mod 256);
    DB.SetKeyValue('large_key', BinVal);
    Value := DB.Get('large_key');
    Assert(Length(Value) = 4096, 'Expected 4096 bytes');
    Assert(Value[0] = $00, 'Expected $00 at start');
    Assert(Value[255] = $FF, 'Expected $FF at index 255');
    Assert(Value[4095] = $FF, 'Expected $FF at end');
    DB.Delete('large_key');
    WriteLn('  OK');

    WriteLn('Test 17: Binary codec — all types...');
    Encoded := EncodeValueNull;
    Assert(DecodeTag(Encoded) = TAG_NULL, 'Expected TAG_NULL');
    Assert(Length(Encoded) = 1, 'Null should be 1 byte');

    Encoded := EncodeValueBool(True);
    Assert(DecodeTag(Encoded) = TAG_BOOL, 'Expected TAG_BOOL');
    Assert(DecodeBool(Encoded) = True, 'Expected True');
    Encoded := EncodeValueBool(False);
    Assert(DecodeBool(Encoded) = False, 'Expected False');

    Encoded := EncodeValueByte(200);
    Assert(DecodeTag(Encoded) = TAG_BYTE, 'Expected TAG_BYTE');
    Assert(DecodeByte(Encoded) = 200, 'Expected 200');

    Encoded := EncodeValueInt16(-30000);
    Assert(DecodeTag(Encoded) = TAG_INT16, 'Expected TAG_INT16');
    Assert(DecodeInt16(Encoded) = -30000, 'Expected -30000');

    Encoded := EncodeValueUInt16(60000);
    Assert(DecodeTag(Encoded) = TAG_UINT16, 'Expected TAG_UINT16');
    Assert(DecodeUInt16(Encoded) = 60000, 'Expected 60000');

    Encoded := EncodeValueInt32(42);
    Assert(DecodeTag(Encoded) = TAG_INT32, 'Expected TAG_INT32');
    Assert(DecodeInt32(Encoded) = 42, 'Expected 42');
    Encoded := EncodeValueInt32(-2147483647);
    Assert(DecodeInt32(Encoded) = -2147483647, 'Expected min int32');

    Encoded := EncodeValueUInt32(4000000000);
    Assert(DecodeTag(Encoded) = TAG_UINT32, 'Expected TAG_UINT32');
    Assert(DecodeUInt32(Encoded) = 4000000000, 'Expected 4000000000');

    Encoded := EncodeValueInt64(9223372036854775807);
    Assert(DecodeTag(Encoded) = TAG_INT64, 'Expected TAG_INT64');
    Assert(DecodeInt64(Encoded) = 9223372036854775807, 'Expected max int64');

    Encoded := EncodeValueUInt64(18446744073709551615);
    Assert(DecodeTag(Encoded) = TAG_UINT64, 'Expected TAG_UINT64');
    Assert(DecodeUInt64(Encoded) = 18446744073709551615, 'Expected max uint64');

    Encoded := EncodeValueSingle(2.5);
    Assert(DecodeTag(Encoded) = TAG_SINGLE, 'Expected TAG_SINGLE');
    Assert(Abs(DecodeSingle(Encoded) - 2.5) < 0.001, 'Expected 2.5');

    Encoded := EncodeValueDouble(3.14);
    Assert(DecodeTag(Encoded) = TAG_DOUBLE, 'Expected TAG_DOUBLE');
    Assert(Abs(DecodeDouble(Encoded) - 3.14) < 0.001, 'Expected 3.14');

    Encoded := EncodeValueString('Hello World');
    Assert(DecodeTag(Encoded) = TAG_STRING, 'Expected TAG_STRING');
    Assert(DecodeString(Encoded) = 'Hello World', 'Expected Hello World');
    Encoded := EncodeValueString('');
    Assert(DecodeString(Encoded) = '', 'Expected empty string');

    Encoded := EncodeValueBytes(BytesOf([$01, $02, $03]));
    Assert(DecodeTag(Encoded) = TAG_BYTES, 'Expected TAG_BYTES');
    Value := DecodeBytes(Encoded);
    Assert(Length(Value) = 3, 'Expected 3 bytes');
    Assert(Value[0] = $01, 'Expected $01');
    Assert(Value[2] = $03, 'Expected $03');
    WriteLn('  OK');

    TestDeepNesting(DB);

    WriteLn('Test 23: Handle property...');
    Assert(DB.Handle <> nil, 'Handle should not be nil');
    WriteLn('  OK');

    DB.Free;

    WriteLn('Test 24: Reopen database...');
    DB := TGarryDatabase.Create('test_fpc.db', False);
    Assert(DB.Exists(EncodeKey(['nav', '001'])), 'nav key should persist');
    Value := DB.Get(EncodeKey(['nav', '001']));
    Assert(Length(Value) = 1, 'Expected 1 byte');
    Assert(Value[0] = $01, 'Expected $01');
    DB.Free;
    WriteLn('  OK');

    DeleteFile('test_fpc.db');

    WriteLn('');
    WriteLn('All tests passed!');
  except
    on E: Exception do
      WriteLn('FAILED: ', E.Message);
  end;
end.