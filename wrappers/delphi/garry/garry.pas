unit garry;

{$IFDEF FPC}
  {$MODE DELPHI}
{$ENDIF}

interface

uses
  garry_types;

const
  {$IFDEF MSWINDOWS}
  GARRY_LIB = 'garry.dll';
  {$ELSE}
  {$IFDEF DARWIN}
  GARRY_LIB = 'libgarry.dylib';
  {$ELSE}
  GARRY_LIB = 'libgarry.so';
  {$ENDIF}
  {$ENDIF}

function garry_config_default: TGarryConfig; cdecl;
function garry_database_create(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
function garry_database_create_with_config(Path: PAnsiChar;
  const Config: TGarryConfig): PGarryDatabaseHandle; cdecl;
function garry_database_open(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
procedure garry_database_close(DB: PGarryDatabaseHandle); cdecl;

function garry_txn_begin(DB: PGarryDatabaseHandle): Integer; cdecl;
procedure garry_txn_commit(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;
procedure garry_txn_rollback(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;

function garry_get(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
function garry_set(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  const Value: PByte; ValueLen: Integer): TGarryStatus; cdecl;
function garry_delete(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): TGarryStatus; cdecl;
function garry_get_default(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  const Def: PByte; DefLen: Integer;
  Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
function garry_data(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): Integer; cdecl;

function garry_first(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
function garry_last(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
function garry_next_key(DB: PGarryDatabaseHandle; Txn: Integer;
  const After: PByte; AfterLen: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
function garry_prev_key(DB: PGarryDatabaseHandle; Txn: Integer;
  const Before: PByte; BeforeLen: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
function garry_exists(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): Boolean; cdecl;
function garry_count(DB: PGarryDatabaseHandle; Txn: Integer): Integer; cdecl;

function garry_cursor_open(DB: PGarryDatabaseHandle; Txn: Integer;
  const Prefix: PByte; PrefixLen: Integer): PGarryCursorHandle; cdecl;
function garry_cursor_next(Cursor: PGarryCursorHandle;
  Key: PByte; var KeyLen: Integer;
  Value: PByte; var ValueLen: Integer): Boolean; cdecl;
function garry_cursor_next_key(Cursor: PGarryCursorHandle;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
procedure garry_cursor_close(Cursor: PGarryCursorHandle); cdecl;

procedure garry_for_each(DB: PGarryDatabaseHandle; Txn: Integer;
  const Prefix: PByte; PrefixLen: Integer;
  Visitor: TGarryVisitor; UserData: Pointer); cdecl;

function garry_key_split(Str: PAnsiChar; Delimiter: AnsiChar;
  OutBuf: PByte): Integer; cdecl;
function garry_key_unsplit(const Key: PByte; KeyLen: Integer;
  Delimiter: AnsiChar; OutStr: PAnsiChar; OutSize: Integer): Integer; cdecl;
function garry_make_key(Str: PAnsiChar; OutBuf: PByte): Integer; cdecl;
function garry_make_key_parts(Parts: PPAnsiChar; Count: Integer;
  OutBuf: PByte): Integer; cdecl;

procedure garry_empty_byte_array(OutBuf: PByte); cdecl;
function garry_make_key_2(P1, P2: PAnsiChar): TGarryKeyTuple; cdecl;
function garry_make_key_3(P1, P2, P3: PAnsiChar): TGarryKeyTuple; cdecl;
function garry_make_key_4(P1, P2, P3, P4: PAnsiChar): TGarryKeyTuple; cdecl;
procedure garry_encode_key_tuple(const T: PGarryKeyTuple; OutBuf: PByte); cdecl;
function garry_byte_compare(const A: PByte; ALen: Integer;
  const B: PByte; BLen: Integer): Integer; cdecl;

function garry_set_str(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PAnsiChar; Value: PAnsiChar): TGarryStatus; cdecl;
function garry_get_str(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PAnsiChar; Value: PAnsiChar; ValueSize: Integer): TGarryStatus; cdecl;

function garry_strerror(Status: TGarryStatus): PAnsiChar; cdecl;

type
  TGarryDatabase = class
  private
    FHandle: PGarryDatabaseHandle;
    function GetTxn: Integer;
  public
    constructor Create(const Path: string; CreateNew: Boolean = False);
    destructor Destroy; override;

    function Get(const Key: string): TBytes;
    function Get(const Key: TBytes): TBytes;
    procedure SetKeyValue(const Key: string; const Value: TBytes); overload;
    procedure SetKeyValue(const Key: TBytes; const Value: TBytes); overload;
    function Delete(const Key: string): Boolean;
    function Delete(const Key: TBytes): Boolean;
    function Exists(const Key: string): Boolean;
    function Exists(const Key: TBytes): Boolean;

    function Count: Integer;
    function First: TBytes;
    function Last: TBytes;
    function NextKey(const After: TBytes): TBytes;
    function PrevKey(const Before: TBytes): TBytes;

    function BeginTxn: Integer;
    procedure Commit(Txn: Integer);
    procedure Rollback(Txn: Integer);

    function OpenCursor(const Prefix: TBytes): TGarryCursor;
    function OpenCursorAll: TGarryCursor;

    property Handle: PGarryDatabaseHandle read FHandle;
  end;

  TGarryCursor = class
  private
    FHandle: PGarryCursorHandle;
    FDB: TGarryDatabase;
  public
    constructor Create(AHandle: PGarryCursorHandle; ADB: TGarryDatabase);
    destructor Destroy; override;

    function Next(var Key, Value: TBytes): Boolean;
    function NextKey(var Key: TBytes): Boolean;

    property Handle: PGarryCursorHandle read FHandle;
  end;

function EncodeKey(const Parts: array of AnsiString): TBytes;
function DecodeKey(const Key: TBytes): TArray<AnsiString>;

implementation

uses
  SysUtils
  {$IFDEF FPC}
  , dynlibs
  {$ENDIF}
  ;

var
  GarryLibHandle: THandle;
  _garry_config_default: function: TGarryConfig; cdecl;
  _garry_database_create: function(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
  _garry_database_create_with_config: function(Path: PAnsiChar;
    const Config: TGarryConfig): PGarryDatabaseHandle; cdecl;
  _garry_database_open: function(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
  _garry_database_close: procedure(DB: PGarryDatabaseHandle); cdecl;
  _garry_txn_begin: function(DB: PGarryDatabaseHandle): Integer; cdecl;
  _garry_txn_commit: procedure(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;
  _garry_txn_rollback: procedure(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;
  _garry_get: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer;
    Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
  _garry_set: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer;
    const Value: PByte; ValueLen: Integer): TGarryStatus; cdecl;
  _garry_delete: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer): TGarryStatus; cdecl;
  _garry_get_default: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer;
    const Def: PByte; DefLen: Integer;
    Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
  _garry_data: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer): Integer; cdecl;
  _garry_first: function(DB: PGarryDatabaseHandle; Txn: Integer;
    Key: PByte; var KeyLen: Integer): Boolean; cdecl;
  _garry_last: function(DB: PGarryDatabaseHandle; Txn: Integer;
    Key: PByte; var KeyLen: Integer): Boolean; cdecl;
  _garry_next_key: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const After: PByte; AfterLen: Integer;
    Key: PByte; var KeyLen: Integer): Boolean; cdecl;
  _garry_prev_key: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Before: PByte; BeforeLen: Integer;
    Key: PByte; var KeyLen: Integer): Boolean; cdecl;
  _garry_exists: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Key: PByte; KeyLen: Integer): Boolean; cdecl;
  _garry_count: function(DB: PGarryDatabaseHandle; Txn: Integer): Integer; cdecl;
  _garry_cursor_open: function(DB: PGarryDatabaseHandle; Txn: Integer;
    const Prefix: PByte; PrefixLen: Integer): PGarryCursorHandle; cdecl;
  _garry_cursor_next: function(Cursor: PGarryCursorHandle;
    Key: PByte; var KeyLen: Integer;
    Value: PByte; var ValueLen: Integer): Boolean; cdecl;
  _garry_cursor_next_key: function(Cursor: PGarryCursorHandle;
    Key: PByte; var KeyLen: Integer): Boolean; cdecl;
  _garry_cursor_close: procedure(Cursor: PGarryCursorHandle); cdecl;
  _garry_for_each: procedure(DB: PGarryDatabaseHandle; Txn: Integer;
    const Prefix: PByte; PrefixLen: Integer;
    Visitor: TGarryVisitor; UserData: Pointer); cdecl;
  _garry_key_split: function(Str: PAnsiChar; Delimiter: AnsiChar;
    OutBuf: PByte): Integer; cdecl;
  _garry_key_unsplit: function(const Key: PByte; KeyLen: Integer;
    Delimiter: AnsiChar; OutStr: PAnsiChar; OutSize: Integer): Integer; cdecl;
  _garry_make_key: function(Str: PAnsiChar; OutBuf: PByte): Integer; cdecl;
  _garry_make_key_parts: function(Parts: PPAnsiChar; Count: Integer;
    OutBuf: PByte): Integer; cdecl;
  _garry_empty_byte_array: procedure(OutBuf: PByte); cdecl;
  _garry_make_key_2: function(P1, P2: PAnsiChar): TGarryKeyTuple; cdecl;
  _garry_make_key_3: function(P1, P2, P3: PAnsiChar): TGarryKeyTuple; cdecl;
  _garry_make_key_4: function(P1, P2, P3, P4: PAnsiChar): TGarryKeyTuple; cdecl;
  _garry_encode_key_tuple: procedure(const T: PGarryKeyTuple; OutBuf: PByte); cdecl;
  _garry_byte_compare: function(const A: PByte; ALen: Integer;
    const B: PByte; BLen: Integer): Integer; cdecl;
  _garry_set_str: function(DB: PGarryDatabaseHandle; Txn: Integer;
    Key: PAnsiChar; Value: PAnsiChar): TGarryStatus; cdecl;
  _garry_get_str: function(DB: PGarryDatabaseHandle; Txn: Integer;
    Key: PAnsiChar; Value: PAnsiChar; ValueSize: Integer): TGarryStatus; cdecl;
  _garry_strerror: function(Status: TGarryStatus): PAnsiChar; cdecl;

procedure LoadGarryLib;
begin
  if GarryLibHandle <> 0 then
    Exit;
  GarryLibHandle := {$IFDEF FPC}dynlibs.LoadLibrary{$ELSE}LoadLibrary{$ENDIF}(GARRY_LIB);
  if GarryLibHandle = 0 then
    raise Exception.CreateFmt('Failed to load %s', [GARRY_LIB]);

  @_garry_config_default := GetProcedureAddress(GarryLibHandle, 'garry_config_default');
  @_garry_database_create := GetProcedureAddress(GarryLibHandle, 'garry_database_create');
  @_garry_database_create_with_config := GetProcedureAddress(GarryLibHandle, 'garry_database_create_with_config');
  @_garry_database_open := GetProcedureAddress(GarryLibHandle, 'garry_database_open');
  @_garry_database_close := GetProcedureAddress(GarryLibHandle, 'garry_database_close');
  @_garry_txn_begin := GetProcedureAddress(GarryLibHandle, 'garry_txn_begin');
  @_garry_txn_commit := GetProcedureAddress(GarryLibHandle, 'garry_txn_commit');
  @_garry_txn_rollback := GetProcedureAddress(GarryLibHandle, 'garry_txn_rollback');
  @_garry_get := GetProcedureAddress(GarryLibHandle, 'garry_get');
  @_garry_set := GetProcedureAddress(GarryLibHandle, 'garry_set');
  @_garry_delete := GetProcedureAddress(GarryLibHandle, 'garry_delete');
  @_garry_get_default := GetProcedureAddress(GarryLibHandle, 'garry_get_default');
  @_garry_data := GetProcedureAddress(GarryLibHandle, 'garry_data');
  @_garry_first := GetProcedureAddress(GarryLibHandle, 'garry_first');
  @_garry_last := GetProcedureAddress(GarryLibHandle, 'garry_last');
  @_garry_next_key := GetProcedureAddress(GarryLibHandle, 'garry_next_key');
  @_garry_prev_key := GetProcedureAddress(GarryLibHandle, 'garry_prev_key');
  @_garry_exists := GetProcedureAddress(GarryLibHandle, 'garry_exists');
  @_garry_count := GetProcedureAddress(GarryLibHandle, 'garry_count');
  @_garry_cursor_open := GetProcedureAddress(GarryLibHandle, 'garry_cursor_open');
  @_garry_cursor_next := GetProcedureAddress(GarryLibHandle, 'garry_cursor_next');
  @_garry_cursor_next_key := GetProcedureAddress(GarryLibHandle, 'garry_cursor_next_key');
  @_garry_cursor_close := GetProcedureAddress(GarryLibHandle, 'garry_cursor_close');
  @_garry_for_each := GetProcedureAddress(GarryLibHandle, 'garry_for_each');
  @_garry_key_split := GetProcedureAddress(GarryLibHandle, 'garry_key_split');
  @_garry_key_unsplit := GetProcedureAddress(GarryLibHandle, 'garry_key_unsplit');
  @_garry_make_key := GetProcedureAddress(GarryLibHandle, 'garry_make_key');
  @_garry_make_key_parts := GetProcedureAddress(GarryLibHandle, 'garry_make_key_parts');
  @_garry_empty_byte_array := GetProcedureAddress(GarryLibHandle, 'garry_empty_byte_array');
  @_garry_make_key_2 := GetProcedureAddress(GarryLibHandle, 'garry_make_key_2');
  @_garry_make_key_3 := GetProcedureAddress(GarryLibHandle, 'garry_make_key_3');
  @_garry_make_key_4 := GetProcedureAddress(GarryLibHandle, 'garry_make_key_4');
  @_garry_encode_key_tuple := GetProcedureAddress(GarryLibHandle, 'garry_encode_key_tuple');
  @_garry_byte_compare := GetProcedureAddress(GarryLibHandle, 'garry_byte_compare');
  @_garry_set_str := GetProcedureAddress(GarryLibHandle, 'garry_set_str');
  @_garry_get_str := GetProcedureAddress(GarryLibHandle, 'garry_get_str');
  @_garry_strerror := GetProcedureAddress(GarryLibHandle, 'garry_strerror');
end;

function garry_config_default: TGarryConfig; cdecl;
begin
  LoadGarryLib;
  if Assigned(_garry_config_default) then
    Result := _garry_config_default
  else
    FillChar(Result, SizeOf(Result), 0);
end;

function garry_database_create(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
begin
  LoadGarryLib;
  if Assigned(_garry_database_create) then
    Result := _garry_database_create(Path)
  else
    Result := nil;
end;

function garry_database_create_with_config(Path: PAnsiChar;
  const Config: TGarryConfig): PGarryDatabaseHandle; cdecl;
begin
  LoadGarryLib;
  if Assigned(_garry_database_create_with_config) then
    Result := _garry_database_create_with_config(Path, Config)
  else
    Result := nil;
end;

function garry_database_open(Path: PAnsiChar): PGarryDatabaseHandle; cdecl;
begin
  LoadGarryLib;
  if Assigned(_garry_database_open) then
    Result := _garry_database_open(Path)
  else
    Result := nil;
end;

procedure garry_database_close(DB: PGarryDatabaseHandle); cdecl;
begin
  if Assigned(_garry_database_close) then
    _garry_database_close(DB);
end;

function garry_txn_begin(DB: PGarryDatabaseHandle): Integer; cdecl;
begin
  if Assigned(_garry_txn_begin) then
    Result := _garry_txn_begin(DB)
  else
    Result := 0;
end;

procedure garry_txn_commit(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;
begin
  if Assigned(_garry_txn_commit) then
    _garry_txn_commit(DB, Txn);
end;

procedure garry_txn_rollback(DB: PGarryDatabaseHandle; Txn: Integer); cdecl;
begin
  if Assigned(_garry_txn_rollback) then
    _garry_txn_rollback(DB, Txn);
end;

function garry_get(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
begin
  if Assigned(_garry_get) then
    Result := _garry_get(DB, Txn, Key, KeyLen, Value, ValueLen)
  else
    Result := GarryErrIO;
end;

function garry_set(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  const Value: PByte; ValueLen: Integer): TGarryStatus; cdecl;
begin
  if Assigned(_garry_set) then
    Result := _garry_set(DB, Txn, Key, KeyLen, Value, ValueLen)
  else
    Result := GarryErrIO;
end;

function garry_delete(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): TGarryStatus; cdecl;
begin
  if Assigned(_garry_delete) then
    Result := _garry_delete(DB, Txn, Key, KeyLen)
  else
    Result := GarryErrIO;
end;

function garry_get_default(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer;
  const Def: PByte; DefLen: Integer;
  Value: PByte; var ValueLen: Integer): TGarryStatus; cdecl;
begin
  if Assigned(_garry_get_default) then
    Result := _garry_get_default(DB, Txn, Key, KeyLen, Def, DefLen, Value, ValueLen)
  else
    Result := GarryErrIO;
end;

function garry_data(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): Integer; cdecl;
begin
  if Assigned(_garry_data) then
    Result := _garry_data(DB, Txn, Key, KeyLen)
  else
    Result := 0;
end;

function garry_first(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_first) then
    Result := _garry_first(DB, Txn, Key, KeyLen)
  else
    Result := False;
end;

function garry_last(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_last) then
    Result := _garry_last(DB, Txn, Key, KeyLen)
  else
    Result := False;
end;

function garry_next_key(DB: PGarryDatabaseHandle; Txn: Integer;
  const After: PByte; AfterLen: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_next_key) then
    Result := _garry_next_key(DB, Txn, After, AfterLen, Key, KeyLen)
  else
    Result := False;
end;

function garry_prev_key(DB: PGarryDatabaseHandle; Txn: Integer;
  const Before: PByte; BeforeLen: Integer;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_prev_key) then
    Result := _garry_prev_key(DB, Txn, Before, BeforeLen, Key, KeyLen)
  else
    Result := False;
end;

function garry_exists(DB: PGarryDatabaseHandle; Txn: Integer;
  const Key: PByte; KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_exists) then
    Result := _garry_exists(DB, Txn, Key, KeyLen)
  else
    Result := False;
end;

function garry_count(DB: PGarryDatabaseHandle; Txn: Integer): Integer; cdecl;
begin
  if Assigned(_garry_count) then
    Result := _garry_count(DB, Txn)
  else
    Result := 0;
end;

function garry_cursor_open(DB: PGarryDatabaseHandle; Txn: Integer;
  const Prefix: PByte; PrefixLen: Integer): PGarryCursorHandle; cdecl;
begin
  if Assigned(_garry_cursor_open) then
    Result := _garry_cursor_open(DB, Txn, Prefix, PrefixLen)
  else
    Result := nil;
end;

function garry_cursor_next(Cursor: PGarryCursorHandle;
  Key: PByte; var KeyLen: Integer;
  Value: PByte; var ValueLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_cursor_next) then
    Result := _garry_cursor_next(Cursor, Key, KeyLen, Value, ValueLen)
  else
    Result := False;
end;

function garry_cursor_next_key(Cursor: PGarryCursorHandle;
  Key: PByte; var KeyLen: Integer): Boolean; cdecl;
begin
  if Assigned(_garry_cursor_next_key) then
    Result := _garry_cursor_next_key(Cursor, Key, KeyLen)
  else
    Result := False;
end;

procedure garry_cursor_close(Cursor: PGarryCursorHandle); cdecl;
begin
  if Assigned(_garry_cursor_close) then
    _garry_cursor_close(Cursor);
end;

procedure garry_for_each(DB: PGarryDatabaseHandle; Txn: Integer;
  const Prefix: PByte; PrefixLen: Integer;
  Visitor: TGarryVisitor; UserData: Pointer); cdecl;
begin
  if Assigned(_garry_for_each) then
    _garry_for_each(DB, Txn, Prefix, PrefixLen, Visitor, UserData);
end;

function garry_key_split(Str: PAnsiChar; Delimiter: AnsiChar;
  OutBuf: PByte): Integer; cdecl;
begin
  if Assigned(_garry_key_split) then
    Result := _garry_key_split(Str, Delimiter, OutBuf)
  else
    Result := 0;
end;

function garry_key_unsplit(const Key: PByte; KeyLen: Integer;
  Delimiter: AnsiChar; OutStr: PAnsiChar; OutSize: Integer): Integer; cdecl;
begin
  if Assigned(_garry_key_unsplit) then
    Result := _garry_key_unsplit(Key, KeyLen, Delimiter, OutStr, OutSize)
  else
    Result := 0;
end;

function garry_make_key(Str: PAnsiChar; OutBuf: PByte): Integer; cdecl;
begin
  if Assigned(_garry_make_key) then
    Result := _garry_make_key(Str, OutBuf)
  else
    Result := 0;
end;

function garry_make_key_parts(Parts: PPAnsiChar; Count: Integer;
  OutBuf: PByte): Integer; cdecl;
begin
  if Assigned(_garry_make_key_parts) then
    Result := _garry_make_key_parts(Parts, Count, OutBuf)
  else
    Result := 0;
end;

procedure garry_empty_byte_array(OutBuf: PByte); cdecl;
begin
  if Assigned(_garry_empty_byte_array) then
    _garry_empty_byte_array(OutBuf);
end;

function garry_make_key_2(P1, P2: PAnsiChar): TGarryKeyTuple; cdecl;
begin
  if Assigned(_garry_make_key_2) then
    Result := _garry_make_key_2(P1, P2)
  else
    FillChar(Result, SizeOf(Result), 0);
end;

function garry_make_key_3(P1, P2, P3: PAnsiChar): TGarryKeyTuple; cdecl;
begin
  if Assigned(_garry_make_key_3) then
    Result := _garry_make_key_3(P1, P2, P3)
  else
    FillChar(Result, SizeOf(Result), 0);
end;

function garry_make_key_4(P1, P2, P3, P4: PAnsiChar): TGarryKeyTuple; cdecl;
begin
  if Assigned(_garry_make_key_4) then
    Result := _garry_make_key_4(P1, P2, P3, P4)
  else
    FillChar(Result, SizeOf(Result), 0);
end;

procedure garry_encode_key_tuple(const T: PGarryKeyTuple; OutBuf: PByte); cdecl;
begin
  if Assigned(_garry_encode_key_tuple) then
    _garry_encode_key_tuple(T, OutBuf);
end;

function garry_byte_compare(const A: PByte; ALen: Integer;
  const B: PByte; BLen: Integer): Integer; cdecl;
begin
  if Assigned(_garry_byte_compare) then
    Result := _garry_byte_compare(A, ALen, B, BLen)
  else
    Result := 0;
end;

function garry_set_str(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PAnsiChar; Value: PAnsiChar): TGarryStatus; cdecl;
begin
  if Assigned(_garry_set_str) then
    Result := _garry_set_str(DB, Txn, Key, Value)
  else
    Result := GarryErrIO;
end;

function garry_get_str(DB: PGarryDatabaseHandle; Txn: Integer;
  Key: PAnsiChar; Value: PAnsiChar; ValueSize: Integer): TGarryStatus; cdecl;
begin
  if Assigned(_garry_get_str) then
    Result := _garry_get_str(DB, Txn, Key, Value, ValueSize)
  else
    Result := GarryErrIO;
end;

function garry_strerror(Status: TGarryStatus): PAnsiChar; cdecl;
begin
  if Assigned(_garry_strerror) then
    Result := _garry_strerror(Status)
  else
    Result := 'library not loaded';
end;

{ TGarryDatabase }

constructor TGarryDatabase.Create(const Path: string; CreateNew: Boolean);
var
  P: PAnsiChar;
begin
  LoadGarryLib;
  P := PAnsiChar(AnsiString(Path));
  if CreateNew then
    FHandle := garry_database_create(P)
  else
    FHandle := garry_database_open(P);
  if FHandle = nil then
    raise Exception.CreateFmt('Failed to open database: %s', [Path]);
end;

destructor TGarryDatabase.Destroy;
begin
  if FHandle <> nil then
  begin
    garry_database_close(FHandle);
    FHandle := nil;
  end;
  inherited;
end;

function TGarryDatabase.GetTxn: Integer;
begin
  Result := garry_txn_begin(FHandle);
end;

function TGarryDatabase.Get(const Key: string): TBytes;
var
  K: TBytes;
begin
  K := EncodeKey([AnsiString(Key)]);
  Result := Get(K);
end;

function TGarryDatabase.Get(const Key: TBytes): TBytes;
var
  Txn: Integer;
  VLen: Integer;
  St: TGarryStatus;
begin
  Txn := GetTxn;
  try
    VLen := GARRY_MAX_RECORD_SIZE;
    SetLength(Result, VLen);
    St := garry_get(FHandle, Txn, @Key[0], Length(Key), @Result[0], VLen);
    if St = GarryOK then
      SetLength(Result, VLen)
    else
      SetLength(Result, 0);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

procedure TGarryDatabase.SetKeyValue(const Key: string; const Value: TBytes);
var
  K: TBytes;
begin
  K := EncodeKey([AnsiString(Key)]);
  SetKeyValue(K, Value);
end;

procedure TGarryDatabase.SetKeyValue(const Key: TBytes; const Value: TBytes);
var
  Txn: Integer;
  St: TGarryStatus;
begin
  Txn := GetTxn;
  try
    if Length(Value) > 0 then
      St := garry_set(FHandle, Txn, @Key[0], Length(Key), @Value[0], Length(Value))
    else
      St := garry_set(FHandle, Txn, @Key[0], Length(Key), nil, 0);
    if St <> GarryOK then
      raise Exception.Create('garry_set failed');
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.Delete(const Key: string): Boolean;
var
  K: TBytes;
begin
  K := EncodeKey([AnsiString(Key)]);
  Result := Delete(K);
end;

function TGarryDatabase.Delete(const Key: TBytes): Boolean;
var
  Txn: Integer;
  St: TGarryStatus;
begin
  Txn := GetTxn;
  try
    St := garry_delete(FHandle, Txn, @Key[0], Length(Key));
    garry_txn_commit(FHandle, Txn);
    Result := St = GarryOK;
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.Exists(const Key: string): Boolean;
var
  K: TBytes;
begin
  K := EncodeKey([AnsiString(Key)]);
  Result := Exists(K);
end;

function TGarryDatabase.Exists(const Key: TBytes): Boolean;
var
  Txn: Integer;
begin
  Txn := GetTxn;
  try
    Result := garry_exists(FHandle, Txn, @Key[0], Length(Key));
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.Count: Integer;
var
  Txn: Integer;
begin
  Txn := GetTxn;
  try
    Result := garry_count(FHandle, Txn);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.First: TBytes;
var
  Txn: Integer;
  K: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen: Integer;
begin
  Txn := GetTxn;
  try
    KLen := GARRY_MAX_KEY_SIZE;
    if garry_first(FHandle, Txn, @K[0], KLen) then
    begin
      SetLength(Result, KLen);
      Move(K[0], Result[0], KLen);
    end
    else
      SetLength(Result, 0);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.Last: TBytes;
var
  Txn: Integer;
  K: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen: Integer;
begin
  Txn := GetTxn;
  try
    KLen := GARRY_MAX_KEY_SIZE;
    if garry_last(FHandle, Txn, @K[0], KLen) then
    begin
      SetLength(Result, KLen);
      Move(K[0], Result[0], KLen);
    end
    else
      SetLength(Result, 0);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.NextKey(const After: TBytes): TBytes;
var
  Txn: Integer;
  K: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen: Integer;
begin
  Txn := GetTxn;
  try
    KLen := GARRY_MAX_KEY_SIZE;
    if garry_next_key(FHandle, Txn, @After[0], Length(After), @K[0], KLen) then
    begin
      SetLength(Result, KLen);
      Move(K[0], Result[0], KLen);
    end
    else
      SetLength(Result, 0);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.PrevKey(const Before: TBytes): TBytes;
var
  Txn: Integer;
  K: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen: Integer;
begin
  Txn := GetTxn;
  try
    KLen := GARRY_MAX_KEY_SIZE;
    if garry_prev_key(FHandle, Txn, @Before[0], Length(Before), @K[0], KLen) then
    begin
      SetLength(Result, KLen);
      Move(K[0], Result[0], KLen);
    end
    else
      SetLength(Result, 0);
    garry_txn_commit(FHandle, Txn);
  except
    garry_txn_rollback(FHandle, Txn);
    raise;
  end;
end;

function TGarryDatabase.BeginTxn: Integer;
begin
  Result := garry_txn_begin(FHandle);
end;

procedure TGarryDatabase.Commit(Txn: Integer);
begin
  garry_txn_commit(FHandle, Txn);
end;

procedure TGarryDatabase.Rollback(Txn: Integer);
begin
  garry_txn_rollback(FHandle, Txn);
end;

function TGarryDatabase.OpenCursor(const Prefix: TBytes): TGarryCursor;
var
  Cur: PGarryCursorHandle;
  Txn: Integer;
  PPrefix: PByte;
  PLen: Integer;
begin
  Txn := GetTxn;
  if (Length(Prefix) > 0) then
  begin
    PPrefix := @Prefix[0];
    PLen := Length(Prefix);
  end
  else
  begin
    PPrefix := nil;
    PLen := 0;
  end;
  Cur := garry_cursor_open(FHandle, Txn, PPrefix, PLen);
  garry_txn_commit(FHandle, Txn);
  Result := TGarryCursor.Create(Cur, Self);
end;

function TGarryDatabase.OpenCursorAll: TGarryCursor;
begin
  Result := OpenCursor(nil);
end;

{ TGarryCursor }

constructor TGarryCursor.Create(AHandle: PGarryCursorHandle; ADB: TGarryDatabase);
begin
  FHandle := AHandle;
  FDB := ADB;
end;

destructor TGarryCursor.Destroy;
begin
  if FHandle <> nil then
  begin
    garry_cursor_close(FHandle);
    FHandle := nil;
  end;
  inherited;
end;

function TGarryCursor.Next(var Key, Value: TBytes): Boolean;
var
  K, V: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen, VLen: Integer;
begin
  KLen := GARRY_MAX_KEY_SIZE;
  VLen := GARRY_MAX_RECORD_SIZE;
  Result := garry_cursor_next(FHandle, @K[0], KLen, @V[0], VLen);
  if Result then
  begin
    SetLength(Key, KLen);
    Move(K[0], Key[0], KLen);
    SetLength(Value, VLen);
    Move(V[0], Value[0], VLen);
  end;
end;

function TGarryCursor.NextKey(var Key: TBytes): Boolean;
var
  K: array[0..GARRY_MAX_KEY_SIZE - 1] of Byte;
  KLen: Integer;
begin
  KLen := GARRY_MAX_KEY_SIZE;
  Result := garry_cursor_next_key(FHandle, @K[0], KLen);
  if Result then
  begin
    SetLength(Key, KLen);
    Move(K[0], Key[0], KLen);
  end;
end;

{ Key encoding helpers }

function EncodeKey(const Parts: array of AnsiString): TBytes;
var
  I, Offset, Len: Integer;
begin
  SetLength(Result, 0);
  for I := Low(Parts) to High(Parts) do
  begin
    Len := Length(Parts[I]);
    Offset := Length(Result);
    SetLength(Result, Offset + 1 + Len);
    Result[Offset] := Byte(Len);
    if Len > 0 then
      Move(Parts[I][1], Result[Offset + 1], Len);
  end;
end;

function DecodeKey(const Key: TBytes): TArray<AnsiString>;
var
  Count, Offset, Len: Integer;
begin
  SetLength(Result, 0);
  Count := 0;
  Offset := 0;
  while Offset < Length(Key) do
  begin
    Len := Key[Offset];
    Inc(Offset);
    if Offset + Len > Length(Key) then
      Break;
    SetLength(Result, Count + 1);
    if Len > 0 then
      SetString(Result[Count], PAnsiChar(@Key[Offset]), Len)
    else
      Result[Count] := '';
    Inc(Count);
    Inc(Offset, Len);
  end;
end;

initialization
  GarryLibHandle := 0;

end.
