CXX        := c++
# CXXFLAGS   := -std=c++20 -Wall -Wextra -Werror
CXXFLAGS   := -std=c++20

NAME       := server

SRC_DIR    := src
INC_DIR    := include
OBJ_DIR    := obj

SRC        := $(wildcard $(SRC_DIR)/*.cpp)
OBJ        := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

INCLUDES   := -I$(INC_DIR)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@
	@echo "Build $(NAME) successfully!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
