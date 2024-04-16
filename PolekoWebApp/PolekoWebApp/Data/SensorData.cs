using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace PolekoWebApp.Data;

public class SensorData
{
    [JsonPropertyName("humidity")] public float Humidity { get; set; }
    [JsonPropertyName("temperature")] public float Temperature { get; set; }
    [NotMapped] [JsonPropertyName("rssi")] public int Rssi { get; set; }
    [JsonIgnore] public long Epoch { get; set; }
    [JsonIgnore] public int SensorId { get; set; }
    [JsonIgnore] public Sensor Sensor { get; set; }
}