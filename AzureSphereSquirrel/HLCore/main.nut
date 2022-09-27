const HTTP_PUT_TEST_ENDPOINT = "https://eoz525w4wyveqd6.m.pipedream.net/";
const HTTP_GET_SOURCE_TEST_ENDPOINT = "https://eo5p7ama7juiwyp.m.pipedream.net/";

// Say Hello
print("HELLLLLOOOOO!\n");

// Configure the LED and start a blinky
hardware.pin8.configure(DIGITAL_OUT, 0);

local toggle = 0;
function blink()
{
  hlCore.wakeup(1.0, blink);
  toggle += 1; toggle %= 2;
  hardware.pin8.write(toggle);
}
blink();

// Demonstrate async events, schedule a bunch and out-of-order to watch them fire correctly later on
hlCore.wakeup(4.0, function(){ print("Wakeup4.0!\n"); });
hlCore.wakeup(1.0, @() print("Wakeup1.0!\n"));
hlCore.wakeup(2.0, @() print("Wakeup2.0!\n"));
hlCore.wakeup(2.00001, @() print("Wakeup2.00001!\n"));

// Do some simple math so show how flexible the print statement is
print("Some Math: 4 + 5 = " + (4 + 5) + "\n");

// Decode and print some JSON to demonstrate that this feature works
local result = json.decode("{\"myNumber\" : 123}");
prettyPrint.print(result); print("\n");

// Demonstrate both a sync and async put request, we'll trigger async first to show it working
// in the background whilst Squirrel continues to run
local asyncRequest = http.request("PUT", HTTP_PUT_TEST_ENDPOINT, {}, "This is an async PUT request");
result = asyncRequest.sendAsync(function(result)
{
  print("\nAsync: ");
  prettyPrint.print(result); print("\n");
}, @()null, 0);

local syncRequest = http.request("PUT", HTTP_PUT_TEST_ENDPOINT, {}, "This is a sync PUT request");
result = syncRequest.sendSync();
print("\n Sync: "); prettyPrint.print(result); print("\n");

// Use a button triggered async request to download and run arbitrary script on demand
hardware.pin12.configure(DIGITAL_IN);

local buttonPressed = false;
function downloadScript()
{
  if(buttonPressed)
  {
    buttonPressed = hardware.pin12.read() == 1 ? false : true;
  }
  else
  {
    buttonPressed = hardware.pin12.read() == 1 ? false : true;

    if(buttonPressed)
    {
      asyncRequest = http.get(HTTP_GET_SOURCE_TEST_ENDPOINT, {});
      result = asyncRequest.sendAsync(function(result)
      {
        compilestring(result.body)();
      }, @()null, 0);
    }
  }

  hlCore.wakeup(0.5, downloadScript);
}
downloadScript();