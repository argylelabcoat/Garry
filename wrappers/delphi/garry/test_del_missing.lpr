program test_del_missing;
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
  Success: Boolean;
begin
  DeleteFile('test_dm.db');
  DB := TGarryDatabase.Create('test_dm.db', True);
  DB.SetKeyValue('a', BytesOf([$01]));
  WriteLn('about to delete missing key...');
  Flush(Output);
  Success := DB.Delete('also_missing');
  WriteLn('delete missing: ', Success);
  Flush(Output);
  DB.Free;
  WriteLn('done');
  DeleteFile('test_dm.db');
end.
