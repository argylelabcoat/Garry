unit garry_codec;

{$IFDEF FPC}
  {$MODE DELPHI}
{$ENDIF}

interface

uses
  SysUtils;

const
  TAG_NULL    = $00;
  TAG_BOOL    = $01;
  TAG_BYTE    = $02;
  TAG_SBYTE   = $03;
  TAG_INT16   = $04;
  TAG_UINT16  = $05;
  TAG_INT32   = $06;
  TAG_UINT32  = $07;
  TAG_INT64   = $08;
  TAG_UINT64  = $09;
  TAG_SINGLE  = $0A;
  TAG_DOUBLE  = $0B;
  TAG_STRING  = $0C;
  TAG_BYTES   = $0D;
  TAG_DATETIME = $0E;
  TAG_GUID    = $0F;

type
  TGarryVariant = record
    case Integer of
      0: (VBool: Boolean);
      1: (VByte: Byte);
      2: (VSByte: ShortInt);
      3: (VInt16: SmallInt);
      4: (VUInt16: Word);
      5: (VInt32: Integer);
      6: (VUInt32: Cardinal);
      7: (VInt64: Int64);
      8: (VUInt64: UInt64);
      9: (VSingle: Single);
      10: (VDouble: Double);
  end;

function EncodeValueNull: TBytes;
function EncodeValueBool(Value: Boolean): TBytes;
function EncodeValueByte(Value: Byte): TBytes;
function EncodeValueInt16(Value: SmallInt): TBytes;
function EncodeValueUInt16(Value: Word): TBytes;
function EncodeValueInt32(Value: Integer): TBytes;
function EncodeValueUInt32(Value: Cardinal): TBytes;
function EncodeValueInt64(Value: Int64): TBytes;
function EncodeValueUInt64(Value: UInt64): TBytes;
function EncodeValueSingle(Value: Single): TBytes;
function EncodeValueDouble(Value: Double): TBytes;
function EncodeValueString(const Value: string): TBytes;
function EncodeValueBytes(const Value: TBytes): TBytes;

function DecodeTag(const Data: TBytes): Byte;
function DecodeBool(const Data: TBytes): Boolean;
function DecodeByte(const Data: TBytes): Byte;
function DecodeInt16(const Data: TBytes): SmallInt;
function DecodeUInt16(const Data: TBytes): Word;
function DecodeInt32(const Data: TBytes): Integer;
function DecodeUInt32(const Data: TBytes): Cardinal;
function DecodeInt64(const Data: TBytes): Int64;
function DecodeUInt64(const Data: TBytes): UInt64;
function DecodeSingle(const Data: TBytes): Single;
function DecodeDouble(const Data: TBytes): Double;
function DecodeString(const Data: TBytes): string;
function DecodeBytes(const Data: TBytes): TBytes;

implementation

uses
  {$IFDEF FPC}
  FPConvert,
  {$ENDIF}
  Math;

procedure WriteInt16LE(var Buf: TBytes; Offset: Integer; Value: SmallInt);
begin
  Buf[Offset] := Byte(Value and $FF);
  Buf[Offset + 1] := Byte((Value shr 8) and $FF);
end;

procedure WriteUInt16LE(var Buf: TBytes; Offset: Integer; Value: Word);
begin
  Buf[Offset] := Byte(Value and $FF);
  Buf[Offset + 1] := Byte((Value shr 8) and $FF);
end;

procedure WriteInt32LE(var Buf: TBytes; Offset: Integer; Value: Integer);
var
  I: Integer;
begin
  for I := 0 to 3 do
    Buf[Offset + I] := Byte((Value shr (I * 8)) and $FF);
end;

procedure WriteUInt32LE(var Buf: TBytes; Offset: Integer; Value: Cardinal);
var
  I: Integer;
begin
  for I := 0 to 3 do
    Buf[Offset + I] := Byte((Value shr (I * 8)) and $FF);
end;

procedure WriteInt64LE(var Buf: TBytes; Offset: Integer; Value: Int64);
var
  I: Integer;
begin
  for I := 0 to 7 do
    Buf[Offset + I] := Byte((Value shr (I * 8)) and $FF);
end;

procedure WriteUInt64LE(var Buf: TBytes; Offset: Integer; Value: UInt64);
var
  I: Integer;
begin
  for I := 0 to 7 do
    Buf[Offset + I] := Byte((Value shr (I * 8)) and $FF);
end;

procedure WriteSingleLE(var Buf: TBytes; Offset: Integer; Value: Single);
var
  V: Cardinal;
begin
  Move(Value, V, SizeOf(V));
  WriteUInt32LE(Buf, Offset, V);
end;

procedure WriteDoubleLE(var Buf: TBytes; Offset: Integer; Value: Double);
var
  V: UInt64;
begin
  Move(Value, V, SizeOf(V));
  WriteInt64LE(Buf, Offset, V);
end;

function ReadInt16LE(const Buf: TBytes; Offset: Integer): SmallInt;
begin
  Result := SmallInt(Buf[Offset] or (Buf[Offset + 1] shl 8));
end;

function ReadUInt16LE(const Buf: TBytes; Offset: Integer): Word;
begin
  Result := Buf[Offset] or (Buf[Offset + 1] shl 8);
end;

function ReadInt32LE(const Buf: TBytes; Offset: Integer): Integer;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to 3 do
    Result := Result or (Integer(Buf[Offset + I]) shl (I * 8));
end;

function ReadUInt32LE(const Buf: TBytes; Offset: Integer): Cardinal;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to 3 do
    Result := Result or (Cardinal(Buf[Offset + I]) shl (I * 8));
end;

function ReadInt64LE(const Buf: TBytes; Offset: Integer): Int64;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to 7 do
    Result := Result or (Int64(Buf[Offset + I]) shl (I * 8));
end;

function ReadUInt64LE(const Buf: TBytes; Offset: Integer): UInt64;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to 7 do
    Result := Result or (UInt64(Buf[Offset + I]) shl (I * 8));
end;

function ReadSingleLE(const Buf: TBytes; Offset: Integer): Single;
var
  V: Cardinal;
begin
  V := ReadUInt32LE(Buf, Offset);
  Move(V, Result, SizeOf(Result));
end;

function ReadDoubleLE(const Buf: TBytes; Offset: Integer): Double;
var
  V: UInt64;
begin
  V := ReadUInt64LE(Buf, Offset);
  Move(V, Result, SizeOf(Result));
end;

function EncodeValueNull: TBytes;
begin
  SetLength(Result, 1);
  Result[0] := TAG_NULL;
end;

function EncodeValueBool(Value: Boolean): TBytes;
begin
  SetLength(Result, 2);
  Result[0] := TAG_BOOL;
  if Value then
    Result[1] := 1
  else
    Result[1] := 0;
end;

function EncodeValueByte(Value: Byte): TBytes;
begin
  SetLength(Result, 2);
  Result[0] := TAG_BYTE;
  Result[1] := Value;
end;

function EncodeValueInt16(Value: SmallInt): TBytes;
begin
  SetLength(Result, 3);
  Result[0] := TAG_INT16;
  WriteInt16LE(Result, 1, Value);
end;

function EncodeValueUInt16(Value: Word): TBytes;
begin
  SetLength(Result, 3);
  Result[0] := TAG_UINT16;
  WriteUInt16LE(Result, 1, Value);
end;

function EncodeValueInt32(Value: Integer): TBytes;
begin
  SetLength(Result, 5);
  Result[0] := TAG_INT32;
  WriteInt32LE(Result, 1, Value);
end;

function EncodeValueUInt32(Value: Cardinal): TBytes;
begin
  SetLength(Result, 5);
  Result[0] := TAG_UINT32;
  WriteUInt32LE(Result, 1, Value);
end;

function EncodeValueInt64(Value: Int64): TBytes;
begin
  SetLength(Result, 9);
  Result[0] := TAG_INT64;
  WriteInt64LE(Result, 1, Value);
end;

function EncodeValueUInt64(Value: UInt64): TBytes;
begin
  SetLength(Result, 9);
  Result[0] := TAG_UINT64;
  WriteUInt64LE(Result, 1, Value);
end;

function EncodeValueSingle(Value: Single): TBytes;
begin
  SetLength(Result, 5);
  Result[0] := TAG_SINGLE;
  WriteSingleLE(Result, 1, Value);
end;

function EncodeValueDouble(Value: Double): TBytes;
begin
  SetLength(Result, 9);
  Result[0] := TAG_DOUBLE;
  WriteDoubleLE(Result, 1, Value);
end;

function EncodeValueString(const Value: string): TBytes;
var
  Bytes: TBytes;
begin
  Bytes := UTF8Encode(Value);
  SetLength(Result, 1 + Length(Bytes));
  Result[0] := TAG_STRING;
  Move(Bytes[0], Result[1], Length(Bytes));
end;

function EncodeValueBytes(const Value: TBytes): TBytes;
begin
  SetLength(Result, 5 + Length(Value));
  Result[0] := TAG_BYTES;
  WriteInt32LE(Result, 1, Length(Value));
  if Length(Value) > 0 then
    Move(Value[0], Result[5], Length(Value));
end;

function DecodeTag(const Data: TBytes): Byte;
begin
  if Length(Data) = 0 then
    Result := TAG_NULL
  else
    Result := Data[0];
end;

function DecodeBool(const Data: TBytes): Boolean;
begin
  Result := (Length(Data) > 1) and (Data[1] <> 0);
end;

function DecodeByte(const Data: TBytes): Byte;
begin
  if Length(Data) > 1 then
    Result := Data[1]
  else
    Result := 0;
end;

function DecodeInt16(const Data: TBytes): SmallInt;
begin
  if Length(Data) >= 3 then
    Result := ReadInt16LE(Data, 1)
  else
    Result := 0;
end;

function DecodeUInt16(const Data: TBytes): Word;
begin
  if Length(Data) >= 3 then
    Result := ReadUInt16LE(Data, 1)
  else
    Result := 0;
end;

function DecodeInt32(const Data: TBytes): Integer;
begin
  if Length(Data) >= 5 then
    Result := ReadInt32LE(Data, 1)
  else
    Result := 0;
end;

function DecodeUInt32(const Data: TBytes): Cardinal;
begin
  if Length(Data) >= 5 then
    Result := ReadUInt32LE(Data, 1)
  else
    Result := 0;
end;

function DecodeInt64(const Data: TBytes): Int64;
begin
  if Length(Data) >= 9 then
    Result := ReadInt64LE(Data, 1)
  else
    Result := 0;
end;

function DecodeUInt64(const Data: TBytes): UInt64;
begin
  if Length(Data) >= 9 then
    Result := ReadUInt64LE(Data, 1)
  else
    Result := 0;
end;

function DecodeSingle(const Data: TBytes): Single;
begin
  if Length(Data) >= 5 then
    Result := ReadSingleLE(Data, 1)
  else
    Result := 0.0;
end;

function DecodeDouble(const Data: TBytes): Double;
begin
  if Length(Data) >= 9 then
    Result := ReadDoubleLE(Data, 1)
  else
    Result := 0.0;
end;

function DecodeString(const Data: TBytes): string;
begin
  if Length(Data) > 1 then
    Result := UTF8ToString(Copy(Data, 1, Length(Data) - 1))
  else
    Result := '';
end;

function DecodeBytes(const Data: TBytes): TBytes;
var
  Len: Integer;
begin
  if Length(Data) >= 5 then
  begin
    Len := ReadInt32LE(Data, 1);
    if Len > 0 then
    begin
      SetLength(Result, Len);
      Move(Data[5], Result[0], Len);
    end
    else
      SetLength(Result, 0);
  end
  else
    SetLength(Result, 0);
end;

end.
