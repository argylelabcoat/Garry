program garry_test;

{$MODE DELPHI}

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
  WriteLn('Test 12: Deep nesting (10 levels)...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := 'found it';
  Encoded := SerializeDeepRoot(Root);
  Recovered := DeserializeDeepRoot(Encoded);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue = 'found it',
    'Deep value mismatch');
  WriteLn('  OK');

  WriteLn('Test 13: Deep nesting with varied values...');
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

  WriteLn('Test 14: Deep nesting round-trip via database...');
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

  WriteLn('Test 15: Deep nesting empty string...');
  FillChar(Root, SizeOf(Root), 0);
  Root.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue := '';
  Encoded := SerializeDeepRoot(Root);
  Recovered := DeserializeDeepRoot(Encoded);
  Assert(Recovered.Child.Child.Child.Child.Child.Child.Child.Child.Child.Child.DeepValue = '',
    'Empty deep value mismatch');
  WriteLn('  OK');

  WriteLn('Test 16: Deep nesting long string...');
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
  Value: TGarryBytes;
  Encoded: TGarryBytes;
  Parts: array of AnsiString;
  Success: Boolean;
  Txn: Integer;
  Cursor: TGarryCursor;
  Key, Val: TGarryBytes;
  Count: Integer;
begin
  try
    WriteLn('Test 1: Create database...');
    DB := TGarryDatabase.Create('test_fpc.db', True);
    WriteLn('  OK');

    WriteLn('Test 2: Set and get...');
    DB.SetKeyValue('hello', BytesOf([$48, $65, $6C, $6C, $6F]));
    Value := DB.Get('hello');
    Assert(Length(Value) = 5, 'Expected 5 bytes');
    Assert(Value[0] = $48, 'Expected H');
    Assert(Value[4] = $6F, 'Expected o');
    WriteLn('  OK');

    WriteLn('Test 3: Delete...');
    Success := DB.Delete('hello');
    Assert(Success, 'Delete should return true');
    Assert(not DB.Exists('hello'), 'Key should not exist');
    WriteLn('  OK');

    WriteLn('Test 4: Exists...');
    DB.SetKeyValue('test_key', BytesOf([$01, $02, $03]));
    Assert(DB.Exists('test_key'), 'Key should exist');
    DB.Delete('test_key');
    Assert(not DB.Exists('test_key'), 'Key should not exist');
    WriteLn('  OK');

    WriteLn('Test 5: Count...');
    DB.SetKeyValue('a', BytesOf([$01]));
    DB.SetKeyValue('b', BytesOf([$02]));
    DB.SetKeyValue('c', BytesOf([$03]));
    Count := DB.Count;
    Assert(Count = 3, 'Expected 3 keys');
    WriteLn('  OK');

    WriteLn('Test 6: Transaction rollback...');
    Txn := DB.BeginTxn;
    DB.SetKeyValue('txn_test', BytesOf([$AA, $BB]));
    DB.Rollback(Txn);
    Assert(not DB.Exists('txn_test'), 'Key should not exist after rollback');
    WriteLn('  OK');

    WriteLn('Test 7: Cursor iteration...');
    DB.SetKeyValue('prefix/a', BytesOf([$01]));
    DB.SetKeyValue('prefix/b', BytesOf([$02]));
    DB.SetKeyValue('prefix/c', BytesOf([$03]));
    Cursor := DB.OpenCursor(EncodeKey(['prefix']));
    Count := 0;
    while Cursor.Next(Key, Val) do
      Inc(Count);
    Cursor.Free;
    Assert(Count = 3, 'Expected 3 keys with prefix');
    WriteLn('  OK');

    WriteLn('Test 8: Binary codec...');
    Encoded := EncodeValueInt32(42);
    Assert(DecodeTag(Encoded) = TAG_INT32);
    Assert(DecodeInt32(Encoded) = 42);
    Encoded := EncodeValueString('Hello World');
    Assert(DecodeTag(Encoded) = TAG_STRING);
    Assert(DecodeString(Encoded) = 'Hello World');
    Encoded := EncodeValueDouble(3.14);
    Assert(DecodeTag(Encoded) = TAG_DOUBLE);
    Assert(Abs(DecodeDouble(Encoded) - 3.14) < 0.001);
    WriteLn('  OK');

    WriteLn('Test 9: Key encoding...');
    Parts := DecodeKey(EncodeKey(['users', 'matthew', 'docs']));
    Assert(Length(Parts) = 3, 'Expected 3 parts');
    Assert(Parts[0] = 'users');
    Assert(Parts[1] = 'matthew');
    Assert(Parts[2] = 'docs');
    WriteLn('  OK');

    WriteLn('Test 10: First/Last...');
    DB.SetKeyValue('z_last', BytesOf([$FF]));
    Value := DB.First;
    Assert(Length(Value) > 0, 'First should return a key');
    Value := DB.Last;
    Assert(Length(Value) > 0, 'Last should return a key');
    WriteLn('  OK');

    TestDeepNesting(DB);

    DB.Free;

    WriteLn('Test 17: Reopen database...');
    DB := TGarryDatabase.Create('test_fpc.db', False);
    Value := DB.Get('a');
    Assert(Length(Value) = 1, 'Expected 1 byte');
    Assert(Value[0] = $01);
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