# Concept

## Sensor
<!-- * Connect to Wifi and get DHCP address, **IF** wifi not found, start an AP, make it possible to configure network and set DHCP, **tbd** -->
<!-- * Every once upon a time send measurements, make it possible to change the interval, **TCP connection**  -->
* Expose HTTP endpoint, [Return JSON](https://crowcpp.org/master/guides/json/) **Crow** 

## Web App
* Login panel, ability to add users, create users with email and pw, make roles, admin should be able to add new users, [asp.net identity](https://learn.microsoft.com/en-us/aspnet/core/blazor/security/?view=aspnetcore-8.0), [signalr authentication](https://learn.microsoft.com/en-us/aspnet/core/signalr/authn-and-authz?view=aspnetcore-8.0)
* **when not logged in, dont show the layout, instead show login page**
* **don't forget about [handling null on fetching device data](https://learn.microsoft.com/en-us/aspnet/core/blazor/components/lifecycle?view=aspnetcore-8.0#handle-incomplete-async-actions-at-render)**
* Save to mysql
* Add new sensors with either ip or mac
* [Automatically detect sensors in the network](https://stackoverflow.com/questions/48510345/how-to-broadcast-http-request)
* Overview of all sensors' status and wifi quality 
* Display chart and [table](https://learn.microsoft.com/en-us/aspnet/core/blazor/components/quickgrid?view=aspnetcore-8.0), support custom date range, export to .csv or .xls, [maybe virtualization can help with performance](https://learn.microsoft.com/en-us/aspnet/core/blazor/components/virtualization?view=aspnetcore-8.0)
* upon installation, on first launch, configure db