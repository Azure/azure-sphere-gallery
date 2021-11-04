# IoT Hub Recipe Pusher Demonstration (Web)

This sample application is the web app companion to Recipe Pusher Sphere application. This web application consists of two Docker images: `recipe-backend` and `recipe-frontend`. The frontend uses React served by Nginx. The Nginx server also proxies the Flask backend. The Flask application uploads the target file to Azure Blob Storage and updates the device twin in IoT Hub with the file URL, among other attributes.

**IMPORTANT:** This project requires configuration before running. This will allow the backend to communicate with Azure IoT Hub.

## Contents

<!---List file contents of the project, in table.--->

| File/folder | Description |
|-------------|-------------|
| `recipe-backend`       | Backend Flask source code. |
| `recipe-frontend` | Frontend React source code. |
| `docker-compose.yml` | The Docker Compose build file. |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |


## Prerequisites
This application requires the following components:

* Python 3.6 or higher
* Docker 20.10.9 or higher
* Node.JS 14.18.0 or higher
* An Azure subscription with:
  * Blob Storage
  * IoT Hub
  * App Services
  * (optional) Container Registry

## Setup
There are several steps to configure this application.

### Backend and Cloud configuration
A sample configuration file is found in recipe-backend/sample.env. Rename this file to .env before running. The .env file requires the following keys:

* `AZURE_STORAGE_NAME`: The name of the Blob storage account
* `AZURE_IOT_HUB` The name of the IoT Hub

In order to connect to IoT Hub and Blob Storage, both developers and the App Service must have the correct permissions, `IoT Hub Data Contributor` and `Storage Blob Data Contributor`. These can be granted by opening the resource via Azure Portal, selecting *Access Control (IAM)*, and selecting *Add role assignment*.

### Frontend
In production on the frontend, in order to communicate with the backend server, the `proxy_pass` option will need to be modified in `nginx.conf`. This should match the address of the Flask server.

### Container Registry
The Azure Container Registry hostname should be added into the docker-compose.yml file under the `image` attribute of both the frontend and backend images. This will allow easy pushes to the cloud registry after local builds.

## Running the code

### Local development
To run locally, you will need to run both the backend and frontend software. This will require two terminal windows.
#### Backend
1. From the `recipe-backend` directory, create a virtual environment called "venv" with `python3 -m venv venv`.
2. [Activate the virtual environment](https://docs.python.org/3/library/venv.html) using `. venv/bin/activate` (Linux) or `scripts\Scripts\activate.bat` (Windows)
3. Install the dependencies using `pip install -r requirements.txt`
4. Start the server using `flask run`. This will start the backend server on port 5050.

#### Frontend
1. From `recipe-frontend`, run `npm install` to install the dependencies.
2. Run `npm start` to start the frontend development server.
3. From a web browser, navigate to http://localhost:3000/ where you can sign in with the username `superuser` and password `AVeryGoodPassword`.

### Production deployment
1. Build the two backend and frontend Docker images using `docker-compose build`
2. If Azure Container Registry is being used, push an update to the containers. When the images are updated, the corresponding App Service has a webhook to automatically deploy the application.
  1. To deploy to Azure, first login to Azure-CLI with `az login` and then run `az acr login --name <YOUR_CONTAINER_REG>` to login into the container registry.
  2. Push to the Azure Container Repository with `docker push <YOUR_CONTAINER_REG>.azurecr.io/frontend` or `docker push <YOUR_CONTAINER_REG>.azurecr.io/backend`. There are webhooks in Azure to automatically detect a `docker push` and restart the servers.
  
     _Note_: This web application runs in two separate App Services in Azure. Multicontainer apps are not fully supported (Managed Identity, in particular) and will not work for this application.
  
3. If Azure Container Registry is not being used, run `docker-compose up` to start the two containers.


## Key concepts
This application is split into a backend and frontend component. The backend is a basic [Flask](https://flask.palletsprojects.com/en/2.0.x/) application that uses the [Python Azure SDK](https://github.com/Azure/azure-sdk-for-python) to communicate with the Azure servers. The frontend is a [create-react-app](https://reactjs.org/docs/create-a-new-react-app.html) React web app and communicates with the backend using async calls.

## Project expectations
Please consider the following before using this code:

* Flask is being ran directly in the backend. In a production environment, a WCGI server like waitress should be used. Please review the Flask article [_Deploy to Production_](https://flask.palletsprojects.com/en/2.0.x/tutorial/deploy/#run-with-a-production-server).
* The code has not been run through a complete security analysis.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this library, please open a GitHub issue within this repo.


## Contributing

<!--- Include the following text verbatim--->

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

<!---Make sure you've added the [MIT license](https://docs.opensource.microsoft.com/content/releasing/license.html) to the project folder.--->

For details on license, see LICENSE.txt in this directory.
