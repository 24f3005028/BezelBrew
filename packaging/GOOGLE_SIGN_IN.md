# Configure Google once for BezelBrew

This is publisher work, not an end-user setup requirement. The normal Gmail
page offers **Sign in with Google** when a Desktop OAuth identity is available.
An unconfigured build keeps sign-in unavailable and explains who needs to fix it.

1. Create or select your Google Cloud project and enable the Gmail API.
2. Configure the Google Auth Platform consent screen for BezelBrew, including
   the support contact, homepage and privacy information required by Google.
3. Create an OAuth client of type **Desktop app**. Download its client JSON.
   For a private test, add your test Gmail accounts to the project's test users.
4. Build from PowerShell:

   ```powershell
   .\build.ps1 -GoogleClientJson "C:\YourPrivateFolder\bezelbrew-desktop-client.json"
   ```

   Python 3 is used by the build helper to validate this file. It generates a
   header under the ignored `tools/publisher-build` directory and compiles that
   Desktop identity into `dist/BezelBrew.exe`. It prints no credentials. The
   regular build without this parameter remains unconfigured, even if an old
   generated header is present.

5. Before general distribution, complete Google's applicable verification for
   `gmail.metadata`, which Google classifies as a restricted scope. Packaging
   an identity does not make the project verified or remove Google test limits.
6. Test the release on a clean BezelBrew profile: the user should only select
   **Sign in with Google**, choose an account in the browser and give consent.
   No JSON import or Cloud Console work should be required from that user.

The bundled configuration identifies a **public Desktop OAuth client**. Native
applications cannot keep this app identity confidential. It is not a web-server
secret or a user's credential. Never bundle a web client, user access token or
refresh token; the helper rejects web clients and token fields. Keep the source
JSON and generated header out of source control, and ship only the intended
release artifacts.

User refresh tokens remain per Windows user/profile in Windows Credential
Manager. The browser flow uses PKCE, random state and a temporary loopback
callback. No Gmail messages or user tokens are sent to a BezelBrew server.
Personal Desktop-client overrides remain available under Advanced settings.

Sources:

- [Google Desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app)
- [Gmail scopes and verification](https://developers.google.com/workspace/gmail/api/auth/scopes)
