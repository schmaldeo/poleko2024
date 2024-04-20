using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;
using PolekoWebApp.Data;

namespace PolekoWebApp.Components.Services;

public class SensorService(IDbContextFactory<ApplicationDbContext> dbContextFactory, ILogger<SensorService> logger) : BackgroundService
{
    public List<Sensor> Sensors { get; private set; } = [];
    public List<Sensor> SensorsInNetwork { get; private set; } = [];
    private List<Sensor> SensorsToFetch { get; set; } = [];
    private UdpClient? _udpClient;
    private bool _udpRunning;

    public event EventHandler<DisconnectedEventArgs>? Disconnected;

    protected override async Task ExecuteAsync(CancellationToken token)
    {
        Sensors = await GetSensorsFromDb();
        var sensorsWithMacOnly = Sensors.Where(x => x.IpAddress is null).ToArray();
        if (sensorsWithMacOnly.Length != 0)
        { 
            SensorsInNetwork = await GetSensorsFromUdp(false, token);
            var foundSensors = SensorsInNetwork.Where(udp => sensorsWithMacOnly.Any(x => udp.MacAddress == x.MacAddress)).ToList();
            foreach (var sensor in foundSensors)
            {
                var sensorOnFetchList = Sensors.First(x => x.MacAddress == sensor.MacAddress);
                sensorOnFetchList.IpAddress = sensor.IpAddress;
            }
        }
        SensorsToFetch = Sensors.Where(x => x.ManuallyStartFetch == false).ToList();

        List<Task> tasks = [];
        foreach (var sensor in SensorsToFetch)
        {
            tasks.Add(ConnectToSensorAndAddReadingsToDb(sensor, token, 10));
        }

        // refresh sensors from UDP every 5 minutes. better than having it done on client request because in case of
        // heavy traffic it would take ages for some clients to get the return value of that function
        var timer = new Timer(async _ => { await RefreshSensorsInNetwork(new CancellationToken()); },
            null, TimeSpan.Zero, TimeSpan.FromMinutes(5));

        await Task.WhenAll(tasks);
    }

    private async Task<List<Sensor>> GetSensorsFromDb()
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        return await dbContext.Sensors.ToListAsync();
    }

    private async Task<List<Sensor>> GetSensorsFromUdp(bool allowAlreadyAdded, CancellationToken token)
    {
        if (_udpRunning)
        {
            await Task.Delay(TimeSpan.FromSeconds(5), token);
        }

        _udpRunning = true;
        _udpClient = new UdpClient();
        _udpClient.Client.Bind(new IPEndPoint(IPAddress.Any, 5506));
        var cancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);
        cancellationTokenSource.CancelAfter(TimeSpan.FromSeconds(5));
        List<Sensor> sensorsFound = [];
        try
        {
            while (!cancellationTokenSource.Token.IsCancellationRequested)
            {
                var result = await _udpClient.ReceiveAsync(cancellationTokenSource.Token);
                var resultStr = Encoding.UTF8.GetString(result.Buffer);
                var device = JsonSerializer.Deserialize<Sensor>(resultStr)!;
                if (!allowAlreadyAdded)
                {
                    if (!Sensors.Contains(device))
                    {
                        sensorsFound.Add(device);
                    }
                }
                else
                {
                    sensorsFound.Add(device);
                }
            }
        }
        catch (IOException e) when (e.InnerException is SocketException { SocketErrorCode: SocketError.OperationAborted }) { }
        catch (OperationCanceledException) { }
        finally
        {
            _udpClient.Close();
            _udpRunning = false;
        }
        
        return sensorsFound;
    }

    public async Task RefreshSensorsInNetwork(CancellationToken token)
    {
        SensorsInNetwork = await GetSensorsFromUdp(true, token);
    }

    private async Task ConnectToSensorAndAddReadingsToDb(Sensor sensor, CancellationToken token, int bufferSize = 32)
    {
        if (sensor.IpAddress is null) return;

        sensor.TcpClient ??= new TcpClient();

        List<SensorData> readings = [];
        try
        {
            await sensor.TcpClient.ConnectAsync(sensor.IpAddress, 5505, token);
            sensor.Fetching = true;
            var buffer = new byte[1024];
            while (true)
            {
                if (sensor.TcpClient is null) break;
                var bytesRead = await sensor.TcpClient.GetStream().ReadAsync(buffer, token);
                if (bytesRead == 0)
                {
                    break;
                }

                if (token.IsCancellationRequested) break;

                var data = Encoding.UTF8.GetString(buffer, 0, bytesRead);

                var reading = JsonSerializer.Deserialize<SensorData>(data)
                              ?? new SensorData { Temperature = 0, Humidity = 0, Rssi = 0 };
                reading.Epoch = DateTimeOffset.Now.ToUnixTimeSeconds();
                reading.Sensor = sensor;
                sensor.LastReading = reading;
                readings.Add(reading);
                if (readings.Count != bufferSize) continue;
                await AddReadingsToDb(readings);
                readings.Clear();
            }
        }
        catch (IOException e) when (e.InnerException is SocketException { SocketErrorCode: SocketError.OperationAborted })
        {
            logger.LogError($"Stopped fetching from sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
        }
        catch (SocketException e)
        {
            OnDeviceDisconnected(sensor.IpAddress ?? sensor.MacAddress ?? "");
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
            sensor.Fetching = false;
        }
        catch (ObjectDisposedException)
        {
            OnDeviceDisconnected(sensor.IpAddress ?? sensor.MacAddress ?? "");
            logger.LogInformation($"Stopped fetching from sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}");
            sensor.Fetching = false;
        }
        catch (InvalidOperationException)
        {
            OnDeviceDisconnected(sensor.IpAddress ?? sensor.MacAddress ?? "");
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}");
            sensor.Fetching = false;
        }
        finally
        {
            sensor.Fetching = false;
            await AddReadingsToDb(readings);
            sensor.TcpClient?.Dispose();
            sensor.TcpClient = null;
        }
    }

    public async Task ConnectToSensor(Sensor sensor, CancellationToken token)
    {
        // TODO if dhcp
        //  if ip is null get ip from udp
        //  if ip is not null connect
        //      if cannot connect try refresh ip 
        sensor.Fetching = true;
        var sensorInList = Sensors.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        if (sensorInList is null) return;
        SensorsToFetch.Add(sensorInList);
        await ConnectToSensorAndAddReadingsToDb(sensor, token, 5);
    }

    public async Task DisconnectFromSensor(Sensor sensor, CancellationTokenSource cancellationTokenSource)
    {
        sensor.Fetching = false;
        await cancellationTokenSource.CancelAsync();
        sensor.TcpClient?.Close();
        sensor.TcpClient = null;
        var sensorInList = SensorsToFetch.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        if (sensorInList is null) return;
        SensorsToFetch.Remove(sensorInList);
    }
    
    public async Task<int> AddSensorToDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        dbContext.Sensors.Add(sensor);
        await dbContext.SaveChangesAsync();
        Sensors.Add(sensor);
        return sensor.SensorId;
    }
    
    public async Task<int> AddSensorToDb(string? ip, string? mac)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        var sensor = new Sensor
        {
            IpAddress = ip,
            MacAddress = mac,
            UsesDhcp = ip is null
        };
        dbContext.Sensors.Add(sensor);
        await dbContext.SaveChangesAsync();
        Sensors.Add(sensor);
        return sensor.SensorId;
    }

    public async Task RemoveSensorFromDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        Sensors.Remove(sensor);
        SensorsToFetch.Remove(sensor);
        dbContext.Sensors.Remove(sensor);
        await dbContext.SaveChangesAsync();
    }

    private async Task AddReadingsToDb(List<SensorData> readings)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        Sensor? cachedSensor = null;
        foreach (var reading in readings)
        {
            cachedSensor ??= await dbContext.Sensors.FindAsync(reading.Sensor.SensorId);
            if (cachedSensor is null) continue;
            reading.Sensor = cachedSensor;
            dbContext.SensorReadings.Add(reading);
        }
        await dbContext.SaveChangesAsync();
    }

    public async Task<SensorData[]> GetReadingsFromDb(Sensor sensor, DateTime beginDate, DateTime endDate)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        var beginDateEpoch = ((DateTimeOffset)beginDate).ToUnixTimeSeconds();
        var endDateEpoch = ((DateTimeOffset)endDate).ToUnixTimeSeconds();
        return dbContext.SensorReadings
            .Where(x => x.SensorId == sensor.SensorId && x.Epoch >= beginDateEpoch && x.Epoch <= endDateEpoch)
            .ToArray();
    }

    private void OnDeviceDisconnected(string ip)
    {
        var eventArgs = new DisconnectedEventArgs { Address = ip };
        Disconnected?.Invoke(this, eventArgs);
    }
    
}

public class DisconnectedEventArgs : EventArgs
{
    public string? Address { get; init; }
}