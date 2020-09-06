#include <unistd.h>
#include <stdio.h>

int main() {
  printf("Hello World\n");
  int A;
  printf("Enter an int: ");
  scanf("%d", &A);
  printf("You entered: %d\n", A);
  return 0;
}