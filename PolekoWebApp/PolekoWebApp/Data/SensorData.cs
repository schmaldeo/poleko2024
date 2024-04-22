using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;
using Microsoft.EntityFrameworkCore;

namespace PolekoWebApp.Data;

[PrimaryKey(nameof(Epoch), nameof(SensorId))]
public class SensorData
{
    [JsonPropertyName("humidity")] public float Humidity { get; set; }
    [JsonPropertyName("temperature")] public float Temperature { get; set; }
    [NotMapped] [JsonPropertyName("rssi")] public int Rssi { get; set; }
    [NotMapped] [JsonPropertyName("interval")] public int Interval { get; set; }
    [JsonIgnore] public long Epoch { get; set; }
    [ForeignKey(nameof(Sensor))] [Required] [JsonIgnore] public int SensorId { get; set; }
    [JsonIgnore] public Sensor Sensor { get; set; }
}