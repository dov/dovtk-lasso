env = Environment(CFLAGS=['-Wall','-g'])

env.ParseConfig("pkg-config --cflags --libs gtk+-3.0")

env.Program("test-dovtk-lasso",
            ["dovtk-lasso.c",
             "test-dovtk-lasso.c"],
            LIBS=['m']+env['LIBS'])

