program test_reopen_fpc;
{$MODE DELPHI}
{$C+}
uses
  SysUtils, garry, garry_types;

function BytesOf(const Vals: array of Byte): TGarryBytes;
var I: Integer;
begin
  SetLength(Result, Length(Vals));
  for I := 0 to High(Vals) do Result[I] := Vals[I];
end;

var
  DB: TGarryDatabase;
  Value, K, Large: TGarryBytes;
  I: Integer;
begin
  DeleteFile('test_rf.db');
  DB := TGarryDatabase.Create('test_rf.db', True);

  DB.SetKeyValue(EncodeKey(['nav', '001']), BytesOf([$01]));
  DB.SetKeyValue(EncodeKey(['nav', '002']), BytesOf([$02]));
  DB.SetKeyValue(EncodeKey(['nav', '003']), BytesOf([$03]));

  SetLength(Large, 4096);
  for I := 0 to High(Large) do
    Large[I] := Byte(I mod 256);
  DB.SetKeyValue('large_key', Large);

  WriteLn('Before close:');
  K := EncodeKey(['nav', '001']);
  WriteLn('  Exists(nav/001): ', DB.Exists(K));
  WriteLn('  Get(nav/001) len: ', Length(DB.Get(K)));

  DB.Free;

  WriteLn('After reopen:');
  DB := TGarryDatabase.Create('test_rf.db', False);
  K := EncodeKey(['nav', '001']);
  WriteLn('  Exists(nav/001): ', DB.Exists(K));
  Value := DB.Get(K);
  WriteLn('  Get(nav/001) len: ', Length(Value));
  if Length(Value) > 0 then
    WriteLn('  Value[0]: $', HexStr(Value[0], 2));
  DB.Free;

  DeleteFile('test_rf.db');
  WriteLn('Done');
end.
