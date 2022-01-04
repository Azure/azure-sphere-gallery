using Microsoft.Identity.Client;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace DevicePropertiesWebHook
{
    public class ADToken
    {
        // For more information on using the Azure Sphere Public API
        // including the definitions below, go here:
        // https://docs.microsoft.com/en-us/rest/api/azure-sphere/

        /// <summary>
        /// Azure Sphere Public API resource URI
        /// </summary>
        private static readonly List<string> Scopes = new List<string>() { "https://sphere.azure.net/api/user_impersonation" };
        /// <summary>
        /// Azure Sphere Public API client application ID.
        /// </summary>
        private const string ClientApplicationId = "0B1C8F7E-28D2-4378-97E2-7D7D63F7C87F";
        /// <summary>
        /// Azure Sphere Tenant ID.
        /// </summary>
        public const string Tenant = "7d71c83c-ccdf-45b7-b3c9-9c41b94406d9";
        /// <summary>
        ///  The PublicClientApplication
        /// </summary>
        private static IPublicClientApplication pca=null;

        /// <summary>
        /// AuthenticationResult holds the current token.
        /// </summary>
        private static AuthenticationResult authResult = null;

        /// <summary>
        ///  Instructions returned from init of Device Flow Auth
        /// </summary>
        private static string deviceFlowInstructions = string.Empty;
        /// <summary>
        ///  authHasCompleted (bool) initially false, set true is device flow is complete.
        /// </summary>
        private static bool authCompleted = false;

        public static bool isAuthComplete()
        {
            return authCompleted;
        }

        static async Task DeviceFlowThread(ManualResetEvent oSignalEvent, string AzureSphereTenantId)
        {
            authResult = await AcquireByDeviceCodeAsync(pca, oSignalEvent);
            // authResult.Account.Username contains the logged in user.
            // confirm that the user is a contributor or administrator in the Azure Sphere Tenant.
            string userId = authResult.Account.Username;

            // https://prod.core.sphere.azure.net/v2/tenants/{tenantId}/users/{userId}/role

            string UserRoles = Utils.GetAS3Data($"tenants/{AzureSphereTenantId}/users/{userId}/role", authResult.AccessToken);
            Debug.WriteLine(UserRoles);

            RoleRoot roles = JsonConvert.DeserializeObject<RoleRoot>(UserRoles);

            // make sure the user is Contributor or Administrator in the Tenant

            bool UserOk = false;

            foreach (Role role in roles.Roles)
            {
                if (role.TenantId == AzureSphereTenantId)
                {
                    foreach (string s in role.RoleNames)
                    {
                        if (String.Equals(s, "contributor", StringComparison.OrdinalIgnoreCase) || String.Equals(s, "administrator", StringComparison.OrdinalIgnoreCase))
                        {
                            UserOk = true;
                            break;
                        }
                    }
                }
            }

            if (!UserOk)
            {
                authResult = null;
                authCompleted = false;
                InitPCA();  // re-initialize PCA
            }
        }

        static async Task<AuthenticationResult> AcquireByDeviceCodeAsync(IPublicClientApplication pca, ManualResetEvent oSignalEvent)
        {
            authCompleted = false;
            try
            {
                var result = await pca.AcquireTokenWithDeviceCode(Scopes, deviceCodeResult =>
                {
                    // This will print the message on the console which tells the user where to go sign-in using 
                    // a separate browser and the code to enter once they sign in.
                    // The AcquireTokenWithDeviceCode() method will poll the server after firing this
                    // device code callback to look for the successful login of the user via that browser.
                    // This background polling (whose interval and timeout data is also provided as fields in the 
                    // deviceCodeCallback class) will occur until:
                    // * The user has successfully logged in via browser and entered the proper code
                    // * The timeout specified by the server for the lifetime of this code (typically ~15 minutes) has been reached
                    // * The developing application calls the Cancel() method on a CancellationToken sent into the method.
                    //   If this occurs, an OperationCanceledException will be thrown (see catch below for more details).
                    deviceFlowInstructions = deviceCodeResult.Message;
                    oSignalEvent.Set();
                    // Console.WriteLine(deviceCodeResult.Message);
                    return Task.FromResult(0);
                }).ExecuteAsync();


                // Console.WriteLine(result.Account.Username);
                authCompleted = true;
                return result;
            }

            catch (MsalServiceException ex)
            {
                // Kind of errors you could have (in ex.Message)

                // AADSTS50059: No tenant-identifying information found in either the request or implied by any provided credentials.
                // Mitigation: as explained in the message from Azure AD, the authoriy needs to be tenanted. you have probably created
                // your public client application with the following authorities:
                // https://login.microsoftonline.com/common or https://login.microsoftonline.com/organizations

                // AADSTS90133: Device Code flow is not supported under /common or /consumers endpoint.
                // Mitigation: as explained in the message from Azure AD, the authority needs to be tenanted

                // AADSTS90002: Tenant <tenantId or domain you used in the authority> not found. This may happen if there are 
                // no active subscriptions for the tenant. Check with your subscription administrator.
                // Mitigation: if you have an active subscription for the tenant this might be that you have a typo in the 
                // tenantId (GUID) or tenant domain name.
            }
            catch (OperationCanceledException ex)
            {
                // If you use a CancellationToken, and call the Cancel() method on it, then this *may* be triggered
                // to indicate that the operation was cancelled. 
                // See https://docs.microsoft.com/en-us/dotnet/standard/threading/cancellation-in-managed-threads 
                // for more detailed information on how C# supports cancellation in managed threads.
            }
            catch (MsalClientException ex)
            {
                // Possible cause - verification code expired before contacting the server
                // This exception will occur if the user does not manage to sign-in before a time out (15 mins) and the
                // call to `AcquireTokenWithDeviceCode` is not cancelled in between
            }
            return null;
        }

        public static string GetDeviceFlowInstructions()
        {
            return deviceFlowInstructions;
        }

        public static void InitPCA()
        {
            deviceFlowInstructions = string.Empty;
            pca = PublicClientApplicationBuilder
                .Create(ClientApplicationId)
                .WithAuthority(AzureCloudInstance.AzurePublic, Tenant)
                .WithDefaultRedirectUri()
                .Build();
        }

        public static void InitDeviceFlow(ManualResetEvent oSignalEvent, string userTenantId)
        {
            deviceFlowInstructions = string.Empty;
            Thread myNewThread = new Thread(() => DeviceFlowThread(oSignalEvent, userTenantId));
            myNewThread.Start();
        }

        public static async Task<AuthenticationResult> GetAToken()
        {
            if (pca == null)
                return null;

            var accounts = await pca.GetAccountsAsync();

            // All AcquireToken* methods store the tokens in the cache, so check the cache first
            try
            {
                return await pca.AcquireTokenSilent(Scopes, accounts.FirstOrDefault()).ExecuteAsync();
            }
            catch (MsalUiRequiredException ex)
            {
                // No token found in the cache or AAD insists that a form interactive auth is required (e.g. the tenant admin turned on MFA)
                // If you want to provide a more complex user experience, check out ex.Classification 

                // we don't want to kick off a Device Auth Flow here
                // we don't have a listening client to return the flow instructions.
                return null;
            }
        }
    }

    // User Roles

    public class RoleRoot
    {
        public Role[] Roles { get; set; }
    }

    public class Role
    {
        public string TenantId { get; set; }
        public string[] RoleNames { get; set; }
    }

}
