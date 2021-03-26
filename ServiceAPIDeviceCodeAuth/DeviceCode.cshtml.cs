using System.Collections.Generic;
using System.Net.Http;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace SphereOBOTest.Pages
{
    public class DeviceCodeModel : PageModel
    {
        public async Task<IActionResult> OnGetDeviceLogin()
        {
            using (HttpClient client = new HttpClient())
            {
                var formContent = new FormUrlEncodedContent(new[]
                {
                    new KeyValuePair<string, string>("client_id", "0B1C8F7E-28D2-4378-97E2-7D7D63F7C87F"),
                    new KeyValuePair<string, string>("scope", "https://sphere.azure.net/api/user_impersonation")
                });

                var responseMessage = await client.PostAsync($"https://login.microsoftonline.com/7d71c83c-ccdf-45b7-b3c9-9c41b94406d9/oauth2/v2.0/devicecode", formContent);
                var response = await responseMessage.Content.ReadAsStringAsync();
                return  new ContentResult { Content = response };
            }
        }

        public async Task<IActionResult> OnGetToken(string code)
        {
            using (HttpClient client = new HttpClient())
            {
                var formContent = new FormUrlEncodedContent(new[]
                {
                    new KeyValuePair<string, string>("client_id", "0B1C8F7E-28D2-4378-97E2-7D7D63F7C87F"),
                    new KeyValuePair<string, string>("scope", "https://sphere.azure.net/api/user_impersonation"),
                    new KeyValuePair<string, string>("grant_type", "device_code"),
                    new KeyValuePair<string, string>("device_code", code)
                });

                var responseMessage = await client.PostAsync($"https://login.microsoftonline.com/7d71c83c-ccdf-45b7-b3c9-9c41b94406d9/oauth2/v2.0/token", formContent);
                var response = await responseMessage.Content.ReadAsStringAsync();
                return new ContentResult { Content = response };
            }
        }
    }
}
