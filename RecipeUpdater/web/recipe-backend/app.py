import os, uuid
import json
from datetime import datetime, timedelta

from dotenv import load_dotenv

from flask import Flask, Blueprint, request, jsonify
from flask_cors import CORS

from werkzeug.utils import secure_filename

from flask_jwt_extended import create_access_token
from flask_jwt_extended import jwt_required
from flask_jwt_extended import JWTManager

from azure.identity import DefaultAzureCredential
from azure.storage.blob import BlobServiceClient, ResourceTypes, BlobSasPermissions, generate_blob_sas
from azure.iot.hub import IoTHubRegistryManager
from azure.iot.hub.models import Twin, TwinProperties, QuerySpecification


load_dotenv()

default_credential = DefaultAzureCredential(exclude_interactive_browser_credential=False)
blob_service_client = BlobServiceClient(f"https://{os.getenv('AZURE_STORAGE_NAME')}.blob.core.windows.net", default_credential)
iothub_registry_manager = IoTHubRegistryManager.from_token_credential(f"{os.getenv('AZURE_IOT_HUB')}.azure-devices.net", default_credential)

app = Flask(__name__)
app.config["JWT_SECRET_KEY"] = "super-secret-jwt-key"
app.config["JWT_ACCESS_TOKEN_EXPIRES"] = 30 * 86400
app.config['UPLOAD_FOLDER'] = "/tmp/"

CORS(app)
JWTManager(app)

api = Blueprint('api', __name__)

try:
    # create a container in Azure Blob Storage if it doesn't exist
    blob_service_client.create_container("recipes")
except: pass

@api.route('/digital-twins/tag', methods=["POST"])
@jwt_required()
def set_digital_twin_tag():
    target = request.json.get("target", None)
    field = request.json.get("field", None)
    split = field.split(".")
    root = split[0]
    child = split[1]
    value = request.json.get("value", None)
    
    tags = dict()
    tags[root] = dict()
    tags[root][child] = value

    twin = iothub_registry_manager.get_twin(target)
    twin_patch = Twin(tags=tags, properties={})
    twin = iothub_registry_manager.update_twin(target, twin_patch, twin.etag)
    return jsonify(success=True)

@api.route('/digital-twins/cancel_campaign', methods=['POST'])
@jwt_required()
def cancel_campaign():
    target = request.json.get("target", None)

    if target:
        twin = iothub_registry_manager.get_twin(target)
        twin_patch = Twin(tags=twin.tags, properties=TwinProperties(recipe_campaign=None))
        twin = iothub_registry_manager.replace_twin(target, twin_patch, twin.etag)

        return jsonify(success=True)
    return jsonify(success=False), 400

@api.route("/digital-twins/upload", methods=["POST"])
@jwt_required()
def upload_digital_twins():
    file = request.files['file']
    
    targets = json.loads(request.form['targets']) # array of target twins
    startTime = request.form['startTime']
    endTime = request.form['endTime']

    if not file:
        return jsonify(error='no file specified'), 400
    
    orig_filename = secure_filename(file.filename)
    filename = secure_filename(str(uuid.uuid4()))

    
    udk = blob_service_client.get_user_delegation_key(
        key_start_time=datetime.utcnow() - timedelta(hours=1),
        key_expiry_time=datetime.utcnow() + timedelta(hours=1)
    )

    sas_token = generate_blob_sas(
        account_name=blob_service_client.account_name,
        container_name='recipes',
        blob_name=filename,
        user_delegation_key=udk,
        permission=BlobSasPermissions(read=True),
        resource_types=ResourceTypes(object=True),
        expiry=datetime.utcnow() + timedelta(hours=24)
    )
    
    
    blob_client = blob_service_client.get_blob_client(container="recipes", blob=filename)
    res = blob_client.upload_blob(file.read())
    size = file.tell()
    url = f"https://{os.getenv('AZURE_STORAGE_NAME')}.blob.core.windows.net/recipes/{filename}?{sas_token}"

    desired = {
        "recipe_campaign": {
            "uuid": filename,
            "filename": orig_filename,
            "start_date": "", # are these needed?
            "end_date": "",
            "size": size,
            "recipe_url": url
        },
        "download_window": {
            "not_before": startTime,
            "not_after": endTime
        }
    }

    for target in targets:
        # iterate over the targets and update each twin
        twin = iothub_registry_manager.get_twin(target)
        twin_patch = Twin(tags={}, properties=TwinProperties(desired=desired))
        twin = iothub_registry_manager.update_twin(target, twin_patch, twin.etag)

    return jsonify(success=True)

@api.route("/login", methods=["POST"])
def login():
    username = request.json.get("username", None)
    password = request.json.get("password", None)

    if username != "superuser" or password != "AVeryGoodPassword":
        return jsonify({"msg": "Bad username or password"}), 401

    access_token = create_access_token(identity=username)
    
    return jsonify(access_token=access_token)

@api.route("/digital-twins/query", methods=["GET"])
@jwt_required()
def get_digital_twins():
    where = request.args.get('where')
    query_expression = 'SELECT * FROM devices'

    # not really safe, but should be fine for a demo application
    if where is not None and len(where):
        where = where.replace("\\'", "\\\\'") # double escape quotes?
        query_expression = f"{query_expression} WHERE {where}"

    query_result = iothub_registry_manager.query_iot_hub(QuerySpecification(query=query_expression)).items
    return jsonify(list(map(lambda x : x.as_dict(), query_result)))


app.register_blueprint(api, url_prefix='/api')

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True, port=5050)