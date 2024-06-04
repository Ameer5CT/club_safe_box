from telebot import types

admin_keyboard = types.InlineKeyboardMarkup()
admin_keyboard.row_width = 1
admin_keyboard.add(
    types.InlineKeyboardButton("The Box", callback_data="The Box"),
    types.InlineKeyboardButton("Raw Firebase DB", callback_data="Raw Firebase DB")
)


def the_box(members):
    b = types.InlineKeyboardMarkup()
    b.row_width = 2
    b.add(
        types.InlineKeyboardButton("Force lock", callback_data="box_force, lock"),
        types.InlineKeyboardButton("Force unlock", callback_data="box_force, unlock")
    )
    b.add(types.InlineKeyboardButton("-----------------------------------------------------------", callback_data="empty"))
    count = 0
    row = []
    for j in members:

        if j.get('nfc') is None:
            continue

        text = j.get('name')
        emoji = ""
        if j.get('surname') is not None:
            text += " " + j.get('surname')

        if j.get('box_access') == "normal":
            emoji = "✅"
        elif j.get('box_access') == "super":
            emoji = "🌟"
        else:
            emoji = "❌"
        text += "  " + emoji
        member = types.InlineKeyboardButton(text, callback_data="box_access, " + emoji + str(j.get('_id')))
        row.append(member)
        count += 1
        if count % 2 == 0:
            b.add(*row)
            row = []
    if row:
        b.add(*row)
    b.add(types.InlineKeyboardButton("-----------------------------------------------------------", callback_data="empty"))
    b.add(types.InlineKeyboardButton("🔙 Back", callback_data="Back, adminKeyboard"))
    return b
