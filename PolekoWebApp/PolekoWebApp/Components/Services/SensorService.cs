using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;
using MudBlazor;
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
    public event EventHandler<SnackbarEventArgs>? SnackbarMessage;
    

    protected override async Task ExecuteAsync(CancellationToken token)
    {
        Sensors = await GetSensorsFromDb();

        // find IPs of sensors that use DHCP/don't have an IP address saved in the database
        var sensorsWithMacOnly = Sensors.Where(x => x.IpAddress is null).ToArray();
        if (sensorsWithMacOnly.Length != 0)
        { 
            SensorsInNetwork = await GetSensorsFromUdp(false, token);
            var foundSensors = SensorsInNetwork.Where(udp => sensorsWithMacOnly.Any(x => udp.MacAddress == x.MacAddress)).ToList();
            foreach (var sensor in foundSensors)
            {
                var sensorInList = Sensors.First(x => x.MacAddress == sensor.MacAddress);
                sensorInList.IpAddress = sensor.IpAddress;
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

    private async Task ConnectToSensorAndAddReadingsToDb(Sensor sensor, CancellationToken token, int bufferSize)
    {
        if (sensor.IpAddress is null) return;

        sensor.TcpClient ??= new TcpClient();

        List<SensorReading> readings = [];
        try
        {
            await sensor.TcpClient.ConnectAsync(sensor.IpAddress, 5505, token);
            sensor.Fetching = true;
            ShowSnackbarMessage($"Połączono z czujnikiem {GetPreferredParameter(sensor)}", Severity.Success);
            var buffer = new byte[1024];
            while (true)
            {
                if (token.IsCancellationRequested) break;
                if (sensor.TcpClient is null) break;
                int bytesRead;
                try
                {
                    // 15 seconds timeout
                    bytesRead = await sensor.TcpClient.GetStream().ReadAsync(buffer, 0, buffer.Length, token)
                        .WaitAsync(TimeSpan.FromSeconds(sensor.FetchInterval + 15), token);
                }
                catch (TaskCanceledException)
                {
                    break;
                }
                catch (TimeoutException)
                {
                    OnDeviceDisconnected(sensor);
                    break;
                }
                
                if (bytesRead == 0)
                {
                    break;
                }

                var data = Encoding.UTF8.GetString(buffer, 0, bytesRead);

                var reading = JsonSerializer.Deserialize<SensorReading>(data)
                              ?? new SensorReading { Temperature = 0, Humidity = 0, Rssi = 0 };
                reading.Epoch = DateTimeOffset.Now.ToUnixTimeSeconds();
                reading.Sensor = sensor;
                sensor.LastReading = reading;
                sensor.Rssi = reading.Rssi;
                if (reading.Interval != sensor.FetchInterval)
                    sensor.FetchInterval = reading.Interval;
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
            OnDeviceDisconnected(sensor);
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
            sensor.Fetching = false;
            sensor.Error = true;
        }
        catch (ObjectDisposedException)
        {
            OnDeviceDisconnected(sensor);
            logger.LogInformation($"Stopped fetching from sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}");
            sensor.Fetching = false;
            sensor.Error = true;
        }
        catch (InvalidOperationException)
        {
            OnDeviceDisconnected(sensor);
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}");
            sensor.Fetching = false;
            sensor.Error = true;
        }
        finally
        {
            sensor.Fetching = false;
            await AddReadingsToDb(readings);
            sensor.TcpClient?.Dispose();
            sensor.TcpClient = null;
        }
    }

    public async Task ConnectToSensor(Sensor sensor, CancellationToken token, int bufferSize = 32)
    {
        if (sensor.IpAddress is null)
        {
            await UpdateDeviceIp(sensor, token);
            if (!sensor.UsesDhcp)
            {
                var dbContext = await dbContextFactory.CreateDbContextAsync(token);
                var sensorInDb = await dbContext.Sensors.FirstAsync(x => x.MacAddress == sensor.MacAddress, token);
                sensorInDb.IpAddress = sensor.IpAddress;
                await dbContext.SaveChangesAsync(token);
            }
        }
        sensor.Fetching = true;
        var sensorInList = Sensors.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        if (sensorInList is null) return;
        SensorsToFetch.Add(sensorInList);
        await ConnectToSensorAndAddReadingsToDb(sensor, token, bufferSize);
    }

    public async Task DisconnectFromSensor(Sensor sensor, CancellationTokenSource cancellationTokenSource)
    {
        sensor.Fetching = false;
        await cancellationTokenSource.CancelAsync();
        sensor.TcpClient?.Close();
        sensor.TcpClient = null;
        var sensorInList = SensorsToFetch.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        ShowSnackbarMessage($"Odłączono od czujnika {GetPreferredParameter(sensor)}.", Severity.Success);
        if (sensorInList is null) return;
        SensorsToFetch.Remove(sensorInList);
    }

    public async Task ChangeInterval(Sensor sensor, int interval, CancellationToken token)
    {
        if (!sensor.Fetching || sensor.TcpClient is null)
        {
            ShowSnackbarMessage("Nie można zmienić częstotliwości bez połączenia z czujnikiem.", Severity.Warning);
            return;
        }

        try
        {
            var stream = sensor.TcpClient.GetStream();
            var json = $"{{\"interval\": {interval}}}";
            await stream.WriteAsync(Encoding.UTF8.GetBytes(json).ToArray(), token);
            ShowSnackbarMessage($"Pomyślnie zmieniono częstotliwość czujnika {GetPreferredParameter(sensor)}", Severity.Success);
        }
        catch (SocketException e)
        {
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
            OnDeviceDisconnected(sensor);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="sensor"></param>
    /// <param name="token"></param>
    /// <returns>A boolean indicating whether the IP has changed or not</returns>
    /// <exception cref="NotImplementedException"></exception>
    public async Task UpdateDeviceIp(Sensor sensor, CancellationToken token)
    {
        var sensors = await GetSensorsFromUdp(true, token);
        var foundSensor = sensors.FirstOrDefault(x => x.MacAddress == sensor.MacAddress);
        if (foundSensor is not null)
        {
            sensor.IpAddress = foundSensor.IpAddress;
        }
    }
    
    public async Task<int> AddSensorToDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        dbContext.Sensors.Add(sensor);
        await dbContext.SaveChangesAsync();
        Sensors.Add(sensor);
        ShowSnackbarMessage($"Pomyślnie dodano czujnik {GetPreferredParameter(sensor)}", Severity.Success);
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
        ShowSnackbarMessage($"Pomyślnie dodano czujnik {GetPreferredParameter(sensor)}", Severity.Success);
        return sensor.SensorId;
    }

    public async Task RemoveSensorFromDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        Sensors.Remove(sensor);
        SensorsToFetch.Remove(sensor);
        dbContext.Sensors.Remove(sensor);
        await dbContext.SaveChangesAsync();
        ShowSnackbarMessage($"Pomyślnie usunięto czujnik {GetPreferredParameter(sensor)}", Severity.Success);
    }

    private async Task AddReadingsToDb(List<SensorReading> readings)
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

    public async Task<SensorReading[]> GetReadingsFromDb(Sensor sensor, DateTime beginDate, DateTime endDate)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        var beginDateEpoch = ((DateTimeOffset)beginDate).ToUnixTimeSeconds();
        var endDateEpoch = ((DateTimeOffset)endDate).ToUnixTimeSeconds();
        return dbContext.SensorReadings
            .Where(x => x.SensorId == sensor.SensorId && x.Epoch >= beginDateEpoch && x.Epoch <= endDateEpoch)
            .ToArray();
    }

    private void OnDeviceDisconnected(Sensor sensor)
    {
        var eventArgs = new DisconnectedEventArgs { Address = GetPreferredParameter(sensor) };
        Disconnected?.Invoke(this, eventArgs);
    }

    private void ShowSnackbarMessage(string message, Severity severity = Severity.Info)
    {
        var eventArgs = new SnackbarEventArgs { Message = message, Severity = severity};
        SnackbarMessage?.Invoke(this, eventArgs);
    }

    private string GetPreferredParameter(Sensor sensor)
    {
        return (sensor.UsesDhcp ? sensor.MacAddress : sensor.IpAddress) ?? string.Empty;
    }
}

public class DisconnectedEventArgs : EventArgs
{
    public string? Address { get; init; }
}

public class SnackbarEventArgs : EventArgs
{
    public string Message { get; init; } = "";
    public Severity Severity { get; init; } = Severity.Info;
}