unit garry_test_unit;

{$IFDEF FPC}
  {$MODE DELPHI}
{$ENDIF}

interface

uses
  garry_types;

type
  TGarryDatabase = class
    function Get(const Key: string): TBytes; overload;
    function Get(const Key: TBytes): TBytes; overload;
    procedure SetKeyValue(const Key: string; const Value: TBytes); overload;
    procedure SetKeyValue(const Key: TBytes; const Value: TBytes); overload;
  end;

implementation

uses
  SysUtils;

function TGarryDatabase.Get(const Key: string): TBytes;
begin
  SetLength(Result, 1);
end;

function TGarryDatabase.Get(const Key: TBytes): TBytes;
begin
  SetLength(Result, 1);
end;

procedure TGarryDatabase.SetKeyValue(const Key: string; const Value: TBytes);
begin
end;

procedure TGarryDatabase.SetKeyValue(const Key: TBytes; const Value: TBytes);
begin
end;

end.
