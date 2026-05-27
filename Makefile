NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -Iinclude

SRC = \
	src/main.cpp \
	src/Client.cpp \
	src/HttpParser.cpp \
	src/HttpRequest.cpp \
	src/HttpResponse.cpp \
	src/ConfigParser.cpp \
	src/ServerConfig.cpp \
	src/ServerManager.cpp \
	src/getMethod.cpp \
	# src/CgiHandler.cpp \

OBJ_DIR = obj

OBJ = $(SRC:src/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: src/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
