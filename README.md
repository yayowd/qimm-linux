# QIMM

Qimm is a situational linux desktop based on wayland.

Qimm show project in screen, a project group relational data together,
each data has a type, and be rendered by qimm client.

## NOTE

Qimm is constructed on the basis of Weston, include both libraries
and source code.

Qimm call the functions from Weston libraries, if the function is not
suitable, the source code will be copied and edited, e.g. weston_client_start
cannot set argv to client, so copy many code and edit little to support arv
when start client.

In addition, some shared files have been copied intact.

The source code of Weston is used in many places, and it is impossible
to explain them one by one, so they are unified here to explain.

I have learned a lot in Weston, and I would like to express my gratitude.
