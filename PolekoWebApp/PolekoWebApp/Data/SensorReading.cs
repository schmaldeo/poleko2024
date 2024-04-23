using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace PolekoWebApp.Data;

public class SensorReading
{
    [JsonIgnore] [DatabaseGenerated(DatabaseGeneratedOption.Identity)] [Key] public int SensorReadingId { get; set; }
    [JsonPropertyName("humidity")] public float Humidity { get; set; }
    [JsonPropertyName("temperature")] public float Temperature { get; set; }
    [NotMapped] [JsonPropertyName("rssi")] public int Rssi { get; set; }
    [NotMapped] [JsonPropertyName("interval")] public int Interval { get; set; }
    [JsonIgnore] public long Epoch { get; set; }
    [ForeignKey(nameof(Sensor))] [Required] [JsonIgnore] public int SensorId { get; set; }
    [JsonIgnore] public Sensor Sensor { get; set; }
}