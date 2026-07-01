extends GutTest

func test_database_lifecycle():
	var db = MeowDB.new()
	assert_true(db.open("/tmp/garry_test.db"))
	assert_true(db.close())

func test_set_get_string():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	assert_true(db.set_string("greeting", "hello"))
	assert_eq(db.get_string("greeting"), "hello")
	db.close()

func test_set_get_bytes():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	var data = PackedByteArray([72, 101, 108, 108, 111])
	assert_true(db.set("raw_key", data))
	var result = db.get("raw_key")
	assert_eq(result, data)
	db.close()

func test_set_get_bytes_range_key():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	var value = PackedByteArray([1, 2, 3, 4, 5, 6, 7, 8])
	assert_true(db.set("\x00\xFFbinary\x80", value))
	var result = db.get("\x00\xFFbinary\x80")
	assert_eq(result, value)
	db.close()

func test_delete():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	db.set_string("to_delete", "value")
	assert_true(db.delete("to_delete"))
	assert_false(db.exists("to_delete"))
	db.close()

func test_exists():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	assert_false(db.exists("missing"))
	db.set_string("present", "yes")
	assert_true(db.exists("present"))
	db.close()

func test_transaction():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	db.begin_transaction()
	db.set_string("key", "value")
	db.commit()
	assert_eq(db.get_string("key"), "value")
	db.close()

func test_rollback():
	var db = MeowDB.new()
	db.open("/tmp/garry_test.db")
	db.set_string("pre_rollback", "before")
	db.begin_transaction()
	db.set_string("during", "during")
	db.rollback()
	assert_eq(db.get_string("pre_rollback"), "before")
	assert_eq(db.get_string("during"), "")
	db.close()

func test_query_returns_array():
	var db = MeowDB.new()
	db.open("/tmp/garry_query_test.db")
	db.set_string("prefix_a", "val_a")
	db.set_string("prefix_b", "val_b")
	db.set_string("prefix_c", "val_c")
	db.set("prefix_binary", PackedByteArray([42, 43, 44]))

	var results = db.query("prefix_")
	# results is an Array of PackedByteArray entries
	# each entry = [klen: 4 bytes][key: klen bytes]
	assert_eq(results.size(), 4)

	# Extract keys from entries and verify they exist
	var keys = []
	for entry in results:
		var bytes = entry as PackedByteArray
		var klen = bytes.decode_u32(0)
		var key_str = bytes.slice(4, 4 + klen).get_string_from_utf8()
		keys.append(key_str)

	assert_true(keys.has("prefix_a"))
	assert_true(keys.has("prefix_b"))
	assert_true(keys.has("prefix_c"))
	assert_true(keys.has("prefix_binary"))
	db.close()
