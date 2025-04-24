# chatgpt'd python translation of the arduino code

from enum import Enum, auto
import time

class State(Enum):
    IDLE = auto()
    AUTHENTICATION = auto()
    RFID = auto()
    PASSWORD = auto()
    FINGERPRINT = auto()
    UNLOCK = auto()
    FAILURE = auto()
    EDIT_USERS = auto()
    EDIT_ADD_USER = auto()
    EDIT_DELETE_USER = auto()

class UserData:
    def __init__(self):
        self.fingerprintKey = ""
        self.password = ""
    def clear(self):
        self.fingerprintKey = ""
        self.password = ""

currentState = State.IDLE
currentUser = UserData()

def check_rfid():
    print("Tap RFID Tag")
    return input().strip()

def check_rfid_sd(rfid, user):
    if rfid == "12345":
        user.fingerprintKey = "abcd"
        user.password = "123456"
        return True
    return False

def check_password(pin, user):
    return pin == user.password

def get_fingerprint():
    print("Scan Fingerprint")
    return input().strip()

def check_fingerprint(key, user):
    return key == user.fingerprintKey

def send_key_to_server():
    time.sleep(1)  # simulate delay
    return True

def admin_pass():
    pass_input = input().strip()
    return pass_input == "admin"

def add_user_sd(rfid, user):
    # simulate adding user
    return True

def delete_user_sd(rfid):
    # simulate deleting user
    return True

def main_loop():
    global currentState, currentUser
    while True:
        if currentState == State.IDLE:
            currentUser.clear()
            print("Enter 1 for authentication, 2 for edit user:")
            entry = input().strip()
            if entry == '1':
                currentState = State.AUTHENTICATION
            elif entry == '2':
                currentState = State.EDIT_USERS
            else:
                print("Invalid entry")
                currentState = State.IDLE

        elif currentState == State.AUTHENTICATION:
            print("Tap RFID card")
            currentState = State.RFID

        elif currentState == State.RFID:
            rfid = check_rfid()
            if check_rfid_sd(rfid, currentUser):
                currentState = State.PASSWORD
            else:
                currentState = State.FAILURE

        elif currentState == State.PASSWORD:
            print("Enter 6-digit PIN:")
            pin = input().strip()
            if check_password(pin, currentUser):
                currentState = State.FINGERPRINT
            else:
                currentState = State.FAILURE

        elif currentState == State.FINGERPRINT:
            print("Scan fingerprint")
            fpKey = get_fingerprint()
            if check_fingerprint(fpKey, currentUser):
                currentState = State.UNLOCK
            else:
                currentState = State.FAILURE

        elif currentState == State.UNLOCK:
            print("Unlocking... Sending key to server")
            if send_key_to_server():
                print("Access granted")
                currentState = State.IDLE

        elif currentState == State.FAILURE:
            print("Authentication failed")
            currentState = State.IDLE

        elif currentState == State.EDIT_USERS:
            print("Enter admin password:")
            if admin_pass():
                print("Press 1 to add user, 2 to delete user:")
                adminEntry = input().strip()
                if adminEntry == '1':
                    currentState = State.EDIT_ADD_USER
                elif adminEntry == '2':
                    currentState = State.EDIT_DELETE_USER
                else:
                    print("Invalid entry")
                    currentState = State.EDIT_USERS
            else:
                print("Admin authentication failed")
                currentState = State.IDLE

        elif currentState == State.EDIT_ADD_USER:
            print("Tap new user's RFID tag")
            rfid = check_rfid()
            print("Enter new user's 6-digit PIN:")
            currentUser.password = input().strip()
            print("Scan new user's fingerprint")
            currentUser.fingerprintKey = get_fingerprint()
            if add_user_sd(rfid, currentUser):
                print("User successfully added")
                currentState = State.IDLE
            else:
                print("Failed to add user")

        elif currentState == State.EDIT_DELETE_USER:
            print("Tap RFID tag of user to delete")
            rfid = check_rfid()
            if delete_user_sd(rfid):
                print("User successfully deleted")
                currentState = State.IDLE
            else:
                print("Failed to delete user")

main_loop()
