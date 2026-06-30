using MeowDB;
using UnityEngine;

public class MeowDBDemo : MonoBehaviour
{
    void Start()
    {
        string dbPath = System.IO.Path.Combine(Application.persistentDataPath, "demo.db");

        // Create database
        using (var db = new MeowDatabase(dbPath))
        {
            // Store a simple value
            db.Set("greeting", System.Text.Encoding.UTF8.GetBytes("Hello, MeowDB!"));

            // Retrieve it
            byte[] value = db.Get("greeting");
            if (value != null)
                Debug.Log($"Retrieved: {System.Text.Encoding.UTF8.GetString(value)}");

            // Store an object
            var player = new PlayerSave
            {
                Name = "Hero",
                Level = 42,
                Health = 100.5f,
                Position = new Vector3Save { X = 1.0f, Y = 2.5f, Z = -3.0f }
            };
            db.Set("players/hero", player);

            // Retrieve object
            var loaded = db.Get<PlayerSave>("players/hero");
            if (loaded != null)
                Debug.Log($"Player: {loaded.Name}, Level {loaded.Level}");

            // Query
            var allPlayers = db.Query<PlayerSave>("players");
            Debug.Log($"Found {allPlayers.Count} player(s)");
        }
    }
}

[System.Serializable]
public class PlayerSave
{
    public string Name { get; set; }
    public int Level { get; set; }
    public float Health { get; set; }
    public Vector3Save Position { get; set; }
}

[System.Serializable]
public class Vector3Save
{
    public float X { get; set; }
    public float Y { get; set; }
    public float Z { get; set; }
}
