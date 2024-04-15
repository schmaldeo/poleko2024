using System.Text.Json.Serialization;

namespace PolekoWebApp.Data;

public class SensorData
{
    [JsonPropertyName("humidity")]
    public float Humidity { get; set; }
    [JsonPropertyName("temperature")]
    public float Temperature { get; set; }
    [JsonPropertyName("rssi")]
    public int Rssi { get; set; }
}