#!/usr/bin/python3
from database.db import db

# Define Task object in database
class Task(db.DynamicDocument):
    task_id = db.StringField(required=True)

# Define Register object in database
class Register(db.DynamicDocument):
    task_id = db.StringField()

# Define Pinger object in database
class Pinger(db.DynamicDocument):
    task_id = db.StringField()

# Define Result object in database
class Result(db.DynamicDocument):
    result_id = db.StringField(required=True)

# Define TaskHistory object in database
class TaskHistory(db.DynamicDocument):
    task_object = db.StringField()