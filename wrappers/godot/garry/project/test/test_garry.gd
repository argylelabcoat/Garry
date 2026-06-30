extends GutTest

func test_database_lifecycle():
	var db = MeowDB.new()
	assert_true(db.open("res://test.db"))
	assert_true(db.close())

func test_set_get_string():
	var db = MeowDB.new()
	db.open("res://test.db")
	assert_true(db.set_string("greeting", "hello"))
	assert_eq(db.get_string("greeting"), "hello")
	db.close()

func test_set_get_bytes():
	var db = MeowDB.new()
	db.open("res://test.db")
	var data = PackedByteArray([72, 101, 108, 108, 111])
	assert_true(db.set("raw_key", data))
	var result = db.get("raw_key")
	assert_eq(result, data)
	db.close()

func test_delete():
	var db = MeowDB.new()
	db.open("res://test.db")
	db.set_string("to_delete", "value")
	assert_true(db.delete("to_delete"))
	assert_false(db.exists("to_delete"))
	db.close()

func test_exists():
	var db = MeowDB.new()
	db.open("res://test.db")
	assert_false(db.exists("missing"))
	db.set_string("present", "yes")
	assert_true(db.exists("present"))
	db.close()

func test_transaction():
	var db = MeowDB.new()
	db.open("res://test.db")
	db.begin_transaction()
	db.set_string("key", "value")
	db.commit()
	assert_eq(db.get_string("key"), "value")
	db.close()

func test_rollback():
	var db = MeowDB.new()
	db.open("res://test.db")
	db.set_string("pre_rollback", "before")
	db.begin_transaction()
	db.set_string("during", "during")
	db.rollback()
	assert_eq(db.get_string("pre_rollback"), "before")
	assert_eq(db.get_string("during"), "")
	db.close()
