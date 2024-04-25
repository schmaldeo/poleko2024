using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using Microsoft.EntityFrameworkCore;
using MudBlazor;
using PolekoWebApp.Data;

namespace PolekoWebApp.Components.Services;

public class SensorService(IDbContextFactory<ApplicationDbContext> dbContextFactory, ILogger<SensorService> logger)
    : BackgroundService
{
    private UdpClient? _udpClient;
    private bool _udpRunning;
    /// <summary>
    ///     Contains sensors saved in the database. 
    /// </summary>
    public List<Sensor> Sensors { get; private set; } = [];
    /// <summary>
    ///     Contains sensors detected in the network.
    /// </summary>
    public List<Sensor> SensorsInNetwork { get; private set; } = [];
    /// <summary>
    ///     Contains sensors that are currently being monitored.
    /// </summary>
    private List<Sensor> SensorsToFetch { get; set; } = [];

    /// <summary>
    ///     Triggered when a device loses connection.
    /// </summary>
    public event EventHandler<ConnectionLostEventArgs>? DeviceConnectionLost;
    /// <summary>
    ///     Triggered when the service wants to show a snackbar.
    /// </summary>
    public event EventHandler<SnackbarEventArgs>? SnackbarMessage;
    
    /// <inheritdoc cref="ExecuteAsync"/> 
    protected override async Task ExecuteAsync(CancellationToken token)
    {
        Sensors = await GetSensorsFromDb();

        // find IPs of sensors that use DHCP/don't have an IP address saved in the database
        var sensorsWithMacOnly = Sensors.Where(x => x.IpAddress is null).ToArray();
        if (sensorsWithMacOnly.Length != 0)
        {
            SensorsInNetwork = await GetSensorsFromUdp(false, token);
            var foundSensors = SensorsInNetwork
                .Where(udp => sensorsWithMacOnly.Any(x => udp.MacAddress == x.MacAddress)).ToList();
            foreach (var sensor in foundSensors)
            {
                var sensorInList = Sensors.First(x => x.MacAddress == sensor.MacAddress);
                sensorInList.IpAddress = sensor.IpAddress;
            }
        }

        SensorsToFetch = Sensors.Where(x => x.ManuallyStartFetch == false).ToList();

        List<Task> tasks = [];
        foreach (var sensor in SensorsToFetch) tasks.Add(ConnectToSensorAndAddReadingsToDb(sensor, token, 10));

        // refresh sensors from UDP every 5 minutes. better than having it done on client request because in case of
        // heavy traffic it would take ages for some clients to get the return value of that function
        var timer = new Timer(async _ => { await RefreshSensorsInNetwork(new CancellationToken()); },
            null, TimeSpan.Zero, TimeSpan.FromMinutes(5));

        await Task.WhenAll(tasks);
    }

    /// <summary>
    ///     Gets sensors from the database.
    /// </summary>
    /// <returns>List of sensors from the database.</returns>
    private async Task<List<Sensor>> GetSensorsFromDb()
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        return await dbContext.Sensors.ToListAsync();
    }

    /// <summary>
    ///     Detects sensors in the network.
    /// </summary>
    /// <param name="allowAlreadyAdded">
    ///     Determines whether the returned list should contain sensors that are already in <see cref="Sensors"/>
    /// </param>
    /// <param name="token">CancellationToken</param>
    /// <returns>List of sensors detected in the network.</returns>
    private async Task<List<Sensor>> GetSensorsFromUdp(bool allowAlreadyAdded, CancellationToken token)
    {
        if (_udpRunning) await Task.Delay(TimeSpan.FromSeconds(5), token);

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
                    if (!Sensors.Contains(device)) sensorsFound.Add(device);
                }
                else
                {
                    sensorsFound.Add(device);
                }
            }
        }
        catch (IOException e) when (e.InnerException is SocketException
                                    {
                                        SocketErrorCode: SocketError.OperationAborted
                                    })
        {
        }
        catch (OperationCanceledException)
        {
        }
        finally
        {
            _udpClient.Close();
            _udpRunning = false;
        }

        return sensorsFound;
    }

    /// <summary>
    ///     Updates <see cref="SensorsInNetwork"/> and updates IP addresses of sensors in <see cref="Sensors"/>.
    /// </summary>
    /// <param name="token">CancellationToken</param>
    public async Task RefreshSensorsInNetwork(CancellationToken token)
    {
        SensorsInNetwork = await GetSensorsFromUdp(true, token);
        // refresh sensors' IP address, basically like calling UpdateDeviceIp()
        foreach (var networkSensor in SensorsInNetwork)
        {
            var localSensor = Sensors.FirstOrDefault(x => x.MacAddress == networkSensor.MacAddress);
            if (localSensor is not null) localSensor.IpAddress = networkSensor.IpAddress;
        }
    }

    /// <summary>
    ///     Establishes a TCP connection with a sensor, reads the data the sensor sends and adds them to the database.
    /// </summary>
    /// <param name="sensor"><see cref="Sensor"/> to connect to</param>
    /// <param name="token">CancellationToken</param>
    /// <param name="bufferSize">Size of the buffer of sensor readings</param>
    private async Task ConnectToSensorAndAddReadingsToDb(Sensor sensor, CancellationToken token, int bufferSize)
    {
        if (sensor.IpAddress is null) return;

        sensor.TcpClient ??= new TcpClient();

        List<SensorReading> readings = [];
        try
        {
            await sensor.TcpClient.ConnectAsync(sensor.IpAddress, 5505, token);
            sensor.Fetching = true;
            sensor.Error = false;
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
                    throw new SocketException();
                }

                if (bytesRead == 0) break;

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
        catch (IOException e) when (e.InnerException is SocketException
                                    {
                                        SocketErrorCode: SocketError.OperationAborted
                                    })
        {
            logger.LogError($"Stopped fetching from sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
        }
        catch (SocketException e)
        {
            OnDeviceConnectionLost(sensor);
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
            sensor.Fetching = false;
            sensor.Error = true;
        }
        catch (ObjectDisposedException)
        {
            OnDeviceConnectionLost(sensor);
            logger.LogInformation($"Stopped fetching from sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}");
            sensor.Fetching = false;
            sensor.Error = true;
        }
        catch (InvalidOperationException)
        {
            OnDeviceConnectionLost(sensor);
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

    /// <summary>
    ///     Connects to a sensor to read data and add it to the database.
    /// </summary>
    /// <param name="sensor">Sensor to connect to</param>
    /// <param name="token">CancellationToken</param>
    /// <param name="bufferSize">Size of the buffer of sensor readings</param>
    public async Task ConnectToSensor(Sensor sensor, CancellationToken token, int bufferSize = 32)
    {
        // if ip is null, update the IP, then if the sensor doesn't use DHCP also update its IP
        if (sensor.IpAddress is null)
        {
            var deviceFound = await UpdateDeviceIp(sensor, token);
            if (deviceFound && !sensor.UsesDhcp)
            {
                var dbContext = await dbContextFactory.CreateDbContextAsync(token);
                var sensorInDb = await dbContext.Sensors.FirstAsync(x => x.MacAddress == sensor.MacAddress, token);
                sensorInDb.IpAddress = sensor.IpAddress;
                await dbContext.SaveChangesAsync(token);
            }
        }
        
        // adding MAC address to sensor if it hasn't got one in case user ever switches to DHCP
        if (sensor.MacAddress is null)
        {
            _ = UpdateDeviceMac(sensor);
            var dbContext = await dbContextFactory.CreateDbContextAsync(token);
            var sensorInDb = await dbContext.Sensors.FirstAsync(x => x.IpAddress == sensor.IpAddress, token);
            sensorInDb.MacAddress = sensor.MacAddress;
            await dbContext.SaveChangesAsync(token);
        }

        sensor.Fetching = true;
        var sensorInList =
            Sensors.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        if (sensorInList is null) return;
        SensorsToFetch.Add(sensorInList);
        await ConnectToSensorAndAddReadingsToDb(sensor, token, bufferSize);
    }

    /// <summary>
    ///     Disconnects from a sensor and sets its TcpClient property to null.
    /// </summary>
    /// <param name="sensor">Sensor to disconnect from</param>
    /// <param name="cancellationTokenSource"><see cref="CancellationTokenSource"/></param>
    public async Task DisconnectFromSensor(Sensor sensor, CancellationTokenSource cancellationTokenSource)
    {
        sensor.Fetching = false;
        await cancellationTokenSource.CancelAsync();
        sensor.TcpClient?.Close();
        sensor.TcpClient = null;
        var sensorInList =
            SensorsToFetch.FirstOrDefault(x => x.IpAddress == sensor.IpAddress || x.MacAddress == sensor.MacAddress);
        ShowSnackbarMessage($"Odłączono od czujnika {GetPreferredParameter(sensor)}.", Severity.Success);
        if (sensorInList is null) return;
        SensorsToFetch.Remove(sensorInList);
    }

    /// <summary>
    ///     Changes the interval in which the sensor sends the data to a connected TCP client.
    /// </summary>
    /// <param name="sensor">Sensor whose interval to change</param>
    /// <param name="interval">New interval</param>
    /// <param name="token">CancellationToken</param>
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
            ShowSnackbarMessage($"Pomyślnie zmieniono częstotliwość czujnika {GetPreferredParameter(sensor)}",
                Severity.Success);
        }
        catch (SocketException e)
        {
            logger.LogError($"Cannot connect to sensor {sensor.IpAddress ?? sensor.MacAddress ?? ""}\n{e.Message}");
            OnDeviceConnectionLost(sensor);
        }
    }

    /// <summary>
    ///     Updates passed sensor's IP (not directly in the database) if it's detected in the network
    /// </summary>
    /// <param name="sensor">Sensor whose IP to update</param>
    /// <param name="token">CancellationToken</param>
    /// <returns>Boolean indicating whether the sensor was found in the network or not</returns>
    public async Task<bool> UpdateDeviceIp(Sensor sensor, CancellationToken token)
    {
        var sensors = await GetSensorsFromUdp(true, token);
        var foundSensor = sensors.FirstOrDefault(x => x.MacAddress == sensor.MacAddress);
        if (foundSensor is null) return false;
        sensor.IpAddress = foundSensor.IpAddress;
        return true;
    }

    /// <summary>
    ///     Updates passed sensor's MAC address (not directly in the database) if it's detected in the network
    /// </summary>
    /// <param name="sensor">Sensor whose MAC to update</param>
    /// <param name="token">CancellationToken</param>
    /// <returns>Boolean indicating whether the sensor was found in the network or not</returns>
    public bool UpdateDeviceMac(Sensor sensor)
    {
        var foundSensor = SensorsInNetwork.FirstOrDefault(x => x.IpAddress == sensor.IpAddress);
        if (foundSensor is null) return false;
        sensor.MacAddress = foundSensor.MacAddress;
        return true;
    }

    /// <summary>
    ///     Adds a sensor to the database.
    /// </summary>
    /// <param name="sensor">Sensor to add</param>
    /// <returns>SensorId in the database</returns>
    public async Task<int> AddSensorToDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        dbContext.Sensors.Add(sensor);
        await dbContext.SaveChangesAsync();
        Sensors.Add(sensor);
        ShowSnackbarMessage($"Pomyślnie dodano czujnik {GetPreferredParameter(sensor)}", Severity.Success);
        return sensor.SensorId;
    }

    /// <summary>
    ///     Adds a sensor to the database.
    /// </summary>
    /// <param name="ip">IP address of the sensor</param>
    /// <param name="mac">MAC address of the sensor</param>
    /// <returns>SensorId in the database</returns>
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

    /// <summary>
    ///     Removes a sensor from the database.
    /// </summary>
    /// <param name="sensor">Sensor to remove</param>
    public async Task RemoveSensorFromDb(Sensor sensor)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        Sensors.Remove(sensor);
        SensorsToFetch.Remove(sensor);
        dbContext.Sensors.Remove(sensor);
        await dbContext.SaveChangesAsync();
        ShowSnackbarMessage($"Pomyślnie usunięto czujnik {GetPreferredParameter(sensor)}", Severity.Success);
    }

    /// <summary>
    ///     Adds a <see cref="List{T}"/> of <see cref="SensorReading"/> to the database.
    /// </summary>
    /// <param name="readings"><see cref="List{T}"/> of <see cref="SensorReading"/></param>
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

    /// <summary>
    /// Retrieves sensor readings from the database within a specified date range.
    /// </summary>
    /// <param name="sensor">The sensor for which to retrieve readings.</param>
    /// <param name="beginDate">The start date of the range.</param>
    /// <param name="endDate">The end date of the range.</param>
    /// <returns>An array of SensorReading objects that fall within the specified date range for the given sensor.</returns>
    public async Task<SensorReading[]> GetReadingsFromDb(Sensor sensor, DateTime beginDate, DateTime endDate)
    {
        await using var dbContext = await dbContextFactory.CreateDbContextAsync();
        var beginDateEpoch = ((DateTimeOffset)beginDate).ToUnixTimeSeconds();
        var endDateEpoch = ((DateTimeOffset)endDate).ToUnixTimeSeconds();
        return dbContext.SensorReadings
            .Where(x => x.SensorId == sensor.SensorId && x.Epoch >= beginDateEpoch && x.Epoch <= endDateEpoch)
            .ToArray();
    }

    /// <summary>
    ///     Invokes the <see cref="DeviceConnectionLost"/> event.
    /// </summary>
    /// <param name="sensor">Sensor which lost connection</param>
    private void OnDeviceConnectionLost(Sensor sensor)
    {
        var eventArgs = new ConnectionLostEventArgs { Address = GetPreferredParameter(sensor) };
        DeviceConnectionLost?.Invoke(this, eventArgs);
    }

    /// <summary>
    ///     Invokes the <see cref="SnackbarMessage"/> event.
    /// </summary>
    /// <param name="message">Message to display in the snackbar</param>
    /// <param name="severity">Snackbar severity</param>
    private void ShowSnackbarMessage(string message, Severity severity = Severity.Info)
    {
        var eventArgs = new SnackbarEventArgs { Message = message, Severity = severity };
        SnackbarMessage?.Invoke(this, eventArgs);
    }

    /// <summary>
    ///     Gets either the MAC or IP address depending on usage of DHCP.
    /// </summary>
    /// <param name="sensor">Sensor whose parameters to display</param>
    /// <returns>MAC or IP address</returns>
    private string GetPreferredParameter(Sensor sensor)
    {
        return (sensor.UsesDhcp ? sensor.MacAddress : sensor.IpAddress) ?? string.Empty;
    }
}

public class ConnectionLostEventArgs : EventArgs
{
    public string? Address { get; init; }
}

public class SnackbarEventArgs : EventArgs
{
    public string Message { get; init; } = "";
    public Severity Severity { get; init; } = Severity.Info;
}