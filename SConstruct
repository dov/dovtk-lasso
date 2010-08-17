env = Environment()

env.ParseConfig("pkg-config --cflags --libs gtk+-2.0")

env.Program("test-dovtk-lasso",
            ["dovtk-lasso.c",
             "test-dovtk-lasso.c"],
            LIBS=['m']+env['LIBS'])

