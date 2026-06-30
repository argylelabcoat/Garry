unit garry_types;

{$IFDEF FPC}
  {$MODE DELPHI}
{$ENDIF}

interface

const
  GARRY_MAX_KEY_SIZE    = 256;
  GARRY_MAX_SUBSCRIPTS  = 16;
  GARRY_MAX_RECORD_SIZE = 16384;

type
  PGarryByte = ^Byte;
  PPAnsiChar = ^PAnsiChar;
  TGarryByteArray = array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;

  PGarryDatabaseHandle = ^TGarryDatabaseHandle;
  TGarryDatabaseHandle = record
  end;

  PGarryCursorHandle = ^TGarryCursorHandle;
  TGarryCursorHandle = record
  end;

  TGarryStatus = (
    GarryOK                = 0,
    GarryErrNotFound       = 1,
    GarryErrLockConflict   = 2,
    GarryErrIO             = 3,
    GarryErrCorrupt        = 4,
    GarryErrInvalidArg     = 5,
    GarryErrNOMEM          = 6,
    GarryErrBufferTooSmall = 7
  );

  TGarryConfig = record
    PoolSize: Integer;
    MaxRecordSize: Integer;
    PageSize: Integer;
    MaxTxns: Integer;
    MaxVersions: Integer;
    MaxKeySize: Integer;
    MaxSubscripts: Integer;
    UseCompression: Boolean;
    BTreeFlags: Integer;
  end;

  TGarryKeyTuple = record
    Parts: array[0..GARRY_MAX_SUBSCRIPTS - 1] of PAnsiChar;
    Counts: array[0..GARRY_MAX_SUBSCRIPTS - 1] of Integer;
    Count: Integer;
  end;

  TGarryVisitor = procedure(const Key: PByte; KeyLen: Integer;
    const Value: PByte; ValueLen: Integer; UserData: Pointer); cdecl;

function GarryStatusToStr(Status: TGarryStatus): AnsiString;

implementation

function GarryStatusToStr(Status: TGarryStatus): AnsiString;
begin
  case Status of
    GarryOK:                Result := 'OK';
    GarryErrNotFound:       Result := 'not found';
    GarryErrLockConflict:   Result := 'lock conflict';
    GarryErrIO:             Result := 'I/O error';
    GarryErrCorrupt:        Result := 'corrupt';
    GarryErrInvalidArg:     Result := 'invalid argument';
    GarryErrNOMEM:          Result := 'out of memory';
    GarryErrBufferTooSmall: Result := 'buffer too small';
  else
    Result := 'unknown error';
  end;
end;

end.
