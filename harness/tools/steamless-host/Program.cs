// Runs Steamless's unpacker plugins on one file from a 64-bit .NET runtime (Steamless ships as .NET Framework GUI
// and CLI programs; this host loads its plugin assemblies directly). docs/ENVIRONMENT.md.
// Usage: dotnet host.dll <steamless dir> <file>; writes <file>.unpacked.exe. Exit 0 unpacked, 1 failed, 2 no plugin.
using System;
using System.IO;
using System.Linq;
using System.Reflection;

var steamless = args[0];
var file = args[1];
var plugins = Path.Combine(steamless, "Plugins");
AppDomain.CurrentDomain.AssemblyResolve += (sender, request) =>
{
    var path = Path.Combine(plugins, new AssemblyName(request.Name).Name + ".dll");
    return File.Exists(path) ? Assembly.LoadFrom(path) : null;
};

var api = Assembly.LoadFrom(Path.Combine(plugins, "Steamless.API.dll"));
var pluginType = api.GetTypes().First(t => t.Name == "SteamlessPlugin");
var loggingType = api.GetTypes().First(t => t.Name == "LoggingService");
var logging = Activator.CreateInstance(loggingType);
var logEvent = loggingType.GetEvent("AddLogMessage");
if (logEvent != null)
{
    var argumentType = logEvent.EventHandlerType.GetMethod("Invoke").GetParameters()[1].ParameterType;
    var handler = typeof(Log).GetMethod(nameof(Log.Print)).MakeGenericMethod(argumentType);
    logEvent.AddEventHandler(logging, Delegate.CreateDelegate(logEvent.EventHandlerType, handler));
}

var optionsType = api.GetTypes().First(t => t.Name == "SteamlessOptions");
var options = Activator.CreateInstance(optionsType);
optionsType.GetProperty("VerboseOutput")?.SetValue(options, true);

foreach (var path in Directory.GetFiles(plugins, "Steamless.Unpacker.*.dll"))
{
    foreach (var type in Assembly.LoadFrom(path).GetTypes().Where(t => pluginType.IsAssignableFrom(t) && !t.IsAbstract))
    {
        var plugin = Activator.CreateInstance(type);
        type.GetMethod("Initialize").Invoke(plugin, new[] { logging });
        if (!(bool)type.GetMethod("CanProcessFile").Invoke(plugin, new object[] { file }))
        {
            continue;
        }
        Console.WriteLine($"plugin: {type.FullName}");
        var unpacked = (bool)type.GetMethod("ProcessFile").Invoke(plugin, new[] { file, options });
        Console.WriteLine($"result: {unpacked}");
        return unpacked ? 0 : 1;
    }
}
Console.WriteLine("no plugin accepted the file");
return 2;

static class Log
{
    public static void Print<T>(object sender, T message)
    {
        Console.WriteLine(message?.GetType().GetProperty("Message")?.GetValue(message));
    }
}
