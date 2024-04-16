using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;
using PolekoWebApp.Data;

namespace PolekoWebApp.Components.Services;

public class SensorService(IDbContextFactory<ApplicationDbContext> dbContextFactory, ILogger<SensorService> logger) : BackgroundService
{
    public Sensor[] Sensors { get; private set; }
    public List<Sensor> SensorsToFetch { get; private set; }
    private UdpClient? _udpClient;
    private CancellationTokenSource _cancellationTokenSource = new();
    private readonly object _bufferLock = new();

    protected override async Task ExecuteAsync(CancellationToken token)
    {
        Sensors = await GetSensorsFromDb();
        SensorsToFetch = Sensors.Where(x => x.OnlyFetchIfMonitoring == false).ToList();
        var macOnly = SensorsToFetch.Where(x => x.IpAddress is null).ToArray();
        if (macOnly.Length != 0)
        {
            // TODO test
            var udpSensors = await GetSensorsFromUdp();
            var foundSensors = udpSensors.Where(udp => macOnly.Any(x => udp.MacAddress == x.MacAddress)).ToList();
            foreach (var sensor in foundSensors)
            {
                var sensorOnFetchList = SensorsToFetch.First(x => x.MacAddress == sensor.MacAddress);
                sensorOnFetchList.IpAddress = sensor.IpAddress;
            }
        }

        List<Task> tasks = [];
        foreach (var sensor in SensorsToFetch)
        {
            tasks.Add(ConnectToSensorAndAddToDb(sensor, token));
        }

        await Task.WhenAll(tasks);
    }

    private async Task<Sensor[]> GetSensorsFromDb()
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        return await dbContext.Sensors.ToArrayAsync();
    }

    private async Task<List<Sensor>> GetSensorsFromUdp()
    {
        _udpClient = new UdpClient();
        _cancellationTokenSource.CancelAfter(TimeSpan.FromSeconds(5));
        List<Sensor> sensorsFound = [];
        try
        {
            while (!_cancellationTokenSource.Token.IsCancellationRequested)
            {
                var result = await _udpClient.ReceiveAsync(_cancellationTokenSource.Token);
                var resultStr = Encoding.UTF8.GetString(result.Buffer);
                var device = JsonSerializer.Deserialize<Sensor>(resultStr)!;
                if (!Sensors.Contains(device))
                {
                    sensorsFound.Add(device);
                }
            }
        }
        finally
        {
            _udpClient.Close();
        }

        return sensorsFound;
    }

    private async Task ConnectToSensorAndAddToDb(Sensor sensor, CancellationToken token)
    {
        sensor.TcpClient ??= new TcpClient();
        // TODO change size
        sensor.Buffer ??= new Buffer<SensorData>(5);
        if (sensor.IpAddress is null) return;
        try
        {
            await sensor.TcpClient.ConnectAsync(sensor.IpAddress, 5505, token);
            var buffer = new byte[1024];
            while (true)
            {
                var bytesRead = await sensor.TcpClient.GetStream().ReadAsync(buffer, token);
                if (bytesRead == 0 || token.IsCancellationRequested) break;

                var data = Encoding.UTF8.GetString(buffer, 0, bytesRead);

                var reading = JsonSerializer.Deserialize<SensorData>(data)
                               ?? new SensorData { Temperature = 0, Humidity = 0, Rssi = 0 };
                reading.Epoch = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
                logger.LogInformation($"{reading.Temperature} {reading.Humidity} {reading.Epoch}");
                // need a lock because theres a risk of a race condition
                lock (_bufferLock)
                {
                    sensor.Buffer.Add(reading);
                }

                sensor.Buffer.BufferOverflow += async (_, _) => { await AddReadingsToDb(sensor); };
            }
        }
        catch (SocketException e)
        {
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
        }
    }

    private async Task AddReadingsToDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        if (sensor.Buffer is null) return;
        lock (_bufferLock)
        {
            foreach (var reading in sensor.Buffer)
            {
                dbContext.SensorReadings.Add(reading);
            }
        }
    }
}
