NAME = webserv
CXX = c++
RM  = rm -rf
OBJ_FILE = obj

GREEN = \033[0;32m
RED   = \033[0;31m
RESET = \033[0m
ARROW = ✔

INCLUDES = src/Logger \
           src/Client \
           src/Server \
		   src/CGIHandler \
		   src/Request \
		   src/Response \
		   src/Config

CXXFLAGS = -Wall -Werror -Wextra  -std=c++98 $(addprefix -I, $(INCLUDES))

LOGGER = src/Logger/Logger
Client = src/Client/Client
Server = src/Server/Server
CGIHandler= src/CGIHandler/CgiHandler
Request=src/Request/Request
Response=src/Response/Response
Config = src/Config/Config

HEADERS =  $(LOGGER).hpp \
           $(Client).hpp \
           $(Server).hpp \
		   $(CGIHandler).hpp \
		   $(Request).hpp \
		   $(Response).hpp \
		   $(Config)Parser.hpp \
		   $(Config).hpp

          
TEST = src/main.cpp

SRCS = $(LOGGER).cpp \
       $(Client).cpp \
       $(Server).cpp \
	   $(CGIHandler).cpp \
	   $(Request).cpp \
	   $(Response).cpp \
	   src/main.cpp \
	   $(Config)Parser.cpp 


OBJS = $(addprefix $(OBJ_FILE)/, $(SRCS:.cpp=.o))


all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Making $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)Done $(ARROW)$(RESET)"

$(OBJ_FILE)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Deleting $(OBJ_FILE)...$(RESET)"
	@$(RM) $(OBJ_FILE)
	@echo "$(RED)Done $(ARROW)$(RESET)"

fclean: clean
	@echo "$(RED)Deleting $(NAME)...$(RESET)"
	@$(RM) $(NAME)
	@echo "$(RED)Done $(ARROW)$(RESET)"

re: fclean all

.PHONY: all clean fclean re
