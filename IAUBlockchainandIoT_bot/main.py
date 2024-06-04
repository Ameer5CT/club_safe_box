import os
import sys
import telebot
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import json
import keyboard
from requests.exceptions import ConnectionError, ReadTimeout


adminIDs = {'ameer': 00000000} #Admin telegram ID number
membersIDs = {'none': 0}
TOKEN = "BOT_TOKEN"
bot = telebot.TeleBot(TOKEN)

cred = credentials.Certificate("DATABASE_JSON_Certificate.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'DATABASE_URL'
})


@bot.message_handler(commands=['start'])
def start_message(msg):
    bot.send_message(adminIDs.get('ameer'), msg)


@bot.message_handler(commands=['admin'])
def admin_message(msg):
    if msg.chat.type != "private" or msg.chat.id not in adminIDs.values():
        return
    admin_str = "*»»»   Admin   «««*"
    bot.send_message(msg.chat.id, admin_str, reply_markup=keyboard.admin_keyboard, parse_mode="MarkDown")


@bot.callback_query_handler(func=lambda message: True)
def callback(call):
    if call.message.chat.type != "private" or (call.message.chat.id not in adminIDs.values() and call.message.chat.id not in membersIDs.values()):
        return
    if call.data == "empty":
        bot.answer_callback_query(call.id, text="-----------------------------------------------------------")
    if call.message.chat.id not in adminIDs.values():
        return
    if call.data.startswith("Back, "):
        if call.data.endswith("adminKeyboard"):
            bot.edit_message_text("*»»»   Admin   «««*", call.message.chat.id, call.message.id,
                                  parse_mode="MarkDown", reply_markup=keyboard.admin_keyboard)
    if call.data == "The Box":
        bot.edit_message_text("»»»   The Box   «««\n\n"
                              "These are the members who have NFC\n"
                              "❌: No Access\n"
                              "✅: Normal Access\n"
                              "🌟: Super Access",
                              call.message.chat.id, call.message.id,
                              reply_markup=keyboard.the_box(db.reference('members').get()))
    if call.data.startswith("box_force, "):
        force = call.data[11:]
        db.reference("box/force").set(force)
        bot.answer_callback_query(call.id, text="Force " + force)
    if call.data.startswith("box_access, "):
        member_id = call.data[13:]
        emoji = call.data[12]
        if emoji == "❌":
            db.reference("members/" + member_id + "/box_access").set("normal")
        elif emoji == "✅":
            db.reference("members/" + member_id + "/box_access").set("super")
        else:
            db.reference("members/" + member_id + "/box_access").delete()
        bot.edit_message_reply_markup(call.message.chat.id, call.message.id,
                                      reply_markup=keyboard.the_box(db.reference('members').get()))
    if call.data == "Raw Firebase DB":
        bot.send_message(call.message.chat.id, json.dumps(db.reference('/').get(), indent=4))
        bot.delete_message(call.message.chat.id, call.message.id)


try:
    bot.infinity_polling(timeout=10, long_polling_timeout=5)
except (ConnectionError, ReadTimeout) as e:
    bot.send_message(adminIDs.get('ameer'), "❌ System will flush ❌")
    sys.stdout.flush()
    os.execv(sys.argv[0], sys.argv)
else:
    bot.infinity_polling(timeout=10, long_polling_timeout=5)
