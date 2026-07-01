program garry_test;

{$MODE DELPHI}

uses
  SysUtils, garry, garry_types, garry_codec;


function BytesOf(const Vals: array of Byte): TGarryBytes;
var
  I: Integer;
begin
  SetLength(Result, Length(Vals));
  for I := 0 to High(Vals) do
    Result[I] := Vals[I];
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

    DB.Free;

    WriteLn('Test 11: Reopen database...');
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
