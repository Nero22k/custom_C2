#!/usr/bin/python3
import json
import resources

from flask import Flask
from flask_restful import Api
from database.db import initialize_db

# Initialize our Flask app
app = Flask(__name__)

# Configure our database on localhost
app.config['MONGODB_SETTINGS'] = {
    'host': 'mongodb://localhost/mirage'
}

# Initialize our database
initialize_db(app)

# Initialize our API
api = Api(app)

# Define the routes for each of our resources
api.add_resource(resources.Tasks, '/tasks', endpoint='tasks')
api.add_resource(resources.Results, '/results')
api.add_resource(resources.History, '/history')
api.add_resource(resources.Registers, '/reg')

# Start the Flask app in debug mode
if __name__ == '__main__':
    app.run(host='0.0.0.0',debug=True)