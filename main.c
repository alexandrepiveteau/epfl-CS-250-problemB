#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define DEFAULT_CAPACITY 128
#define IMPOSSIBLE -1

typedef struct graph {
  size_t size;
  int *capacity;
  int *degree;
  int **neighbours;
} graph_t;

graph_t *make_graph(size_t size) {

  // TODO : Handle bad mallocs.

  graph_t *ptr = malloc(sizeof(graph_t));
  int *capacity = calloc(size, sizeof(int));
  int *degree = calloc(size, sizeof(int));
  int **neighbours = calloc(size, sizeof(int *));

  for (int i = 0; i < size; i++) {
    capacity[i] = DEFAULT_CAPACITY;
    degree[i] = 0;
    neighbours[i] = calloc(DEFAULT_CAPACITY, sizeof(int));
  }

  ptr->size = size;
  ptr->capacity = capacity;
  ptr->degree = degree;
  ptr->neighbours = neighbours;

  return ptr;
}

/**
 * Ensures that the graph has enough room to accept one more neighbour for the given city.
 * @param graph the graph we want to update.
 * @param city the city we want to increase the capacity of.
 */
void graph_ensure_capacity(graph_t *graph, int city) {
  if (!graph) return;
  if (graph->capacity[city] == graph->degree[city]) {
    graph->neighbours[city] = realloc(graph->neighbours[city], graph->capacity[city] * 2 * sizeof(int));
    graph->capacity[city] *= 2;
  }
}

/**
 * Adds a railway between two cities.
 * @param graph the graph we want to act on.
 * @param from the city we're starting the railway from.
 * @param until the city we're ending the railway on.
 */
void graph_add_railway(graph_t *graph, int from, int until) {
  if (!graph) return;
  graph_ensure_capacity(graph, from);
  graph_ensure_capacity(graph, until);
  graph->neighbours[from][graph->degree[from]] = until;
  graph->neighbours[until][graph->degree[until]] = from;
  graph->degree[from]++;
  graph->degree[until]++;
}

/**
 * Adds an airport to the given city.
 * @param graph the graph we want to act on.
 * @param city the city we're adding an airport to.
 */
void graph_add_airport(graph_t *graph, int city) {
  if (!graph) return;
  graph_add_railway(graph, city, (int) graph->size - 1);
}

/**
 * A dynamic circular buffer, which contains some items and may be iterated in a circular fashion. The buffer has a
 * given capacity, which will increase if it's full. When increased, the items from the buffer will be copied into a
 * new circular buffer.
 */
typedef struct circular_buffer {

  /** The number of items that may be present in the buffer at once. */
  size_t capacity;

  /** The index of the first item. */
  size_t start;

  /** How many items are in the buffer right now. */
  size_t size;

  /** A dynamically allocated array of the circular buffer elements. */
  int *elements;
} circular_buffer_t;

/**
 * Creates a new circular buffer, which the provided capacity and no inner items.
 * @param capacity the capacity of the buffer. Must be strictly positive.
 * @return the pointer to the newly allocated buffer. NULL if an error occurred.
 */
circular_buffer_t *make_circular_buffer(size_t capacity) {
  if (capacity == 0) return NULL;
  circular_buffer_t *ptr = malloc(sizeof(circular_buffer_t));
  int *elements = calloc(capacity, sizeof(int));
  if (!ptr || !elements) {
    free(ptr);
    free(elements);
    return NULL;
  }
  ptr->capacity = capacity;
  ptr->size = 0;
  ptr->start = 0;
  ptr->elements = elements;
  return ptr;
}

/**
 * Enqueues an item at the tail of the circular buffer. This runs in O(n) amortized time, since the buffer may require
 * to grow to accommodate for the newly inserted item.
 * @param buffer the circular buffer to which an item is added.
 * @param element the enqueued element.
 * @return 0, or 1 if an error occurred.
 */
int circular_buffer_enqueue(circular_buffer_t *buffer, int element) {
  if (buffer->capacity == buffer->size) {
    int *space = calloc(buffer->capacity * 2, sizeof(int));
    if (!space) return 1; // We could not increase the buffer capacity.

    // TODO: A bit ugly, but essentially, we can simply duplicate the contents of the buffer rather than calculate good bounds.
    memcpy(space, buffer->elements, buffer->capacity * sizeof(int));
    memcpy(&space[buffer->capacity], buffer->elements, buffer->capacity * sizeof(int));

    // Update the buffer structure.
    buffer->capacity *= 2;
    free(buffer->elements);
    buffer->elements = space;
  }
  size_t index = (buffer->start + buffer->size) % buffer->capacity;
  buffer->elements[index] = element;
  buffer->size++;
  return 0;
}

/**
 * Dequeues the head item from this circular buffer. This will fetch the item, move the start index, and finally
 * decrease the size of the buffer to account for the removal.
 * @param buffer the buffer from which the element is removed.
 * @return the dequeued element.
 */
int circular_buffer_dequeue(circular_buffer_t *buffer) {
  if (buffer->size == 0) raise(SIGSEGV); // We do not expect callers to make this call. This is a bad violation.
  size_t index = buffer->start % buffer->capacity;
  int item = buffer->elements[index];
  buffer->size--;
  buffer->start = (buffer->start + 1) % buffer->capacity;
  return item;
}

int solve(graph_t *graph, int from, int until) {
  circular_buffer_t *queue = make_circular_buffer(DEFAULT_CAPACITY);
  if (!queue) return IMPOSSIBLE;
  int distance = 1;
  bool visited[graph->size];
  memset(visited, 0, graph->size * sizeof(bool));

  circular_buffer_enqueue(queue, from);
  while (queue->size > 0) {
    int head = circular_buffer_dequeue(queue);
    if (head < 0) {
      distance = -head;
    } else if (head == until) {
      return distance - 1;
    } else {
      if (graph->degree[head] > 0) circular_buffer_enqueue(queue, -distance - 1);
      for (int i = 0; i < graph->degree[head]; i++) {
        int city = graph->neighbours[head][i];
        if (!visited[city]) {
          circular_buffer_enqueue(queue, city);
          visited[city] = true;
        }
      }
    }
  }
  return IMPOSSIBLE;
}

#define BUFFER_SIZE (16 * 4096)

// A buffer large enough to store any line we're given.
char input_buffer[BUFFER_SIZE];
char *input_ptr = input_buffer;
char *input_ptr_end = input_buffer + BUFFER_SIZE - 1;

/**
 * Initialize the scanner with some proper values.
 */
void scan_init() {
  input_buffer[BUFFER_SIZE - 1] = '\0'; // Null-terminate the input buffer.
  fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
  input_ptr = input_buffer;
}

/** Parses the next multi-digit integer. */
int scan_int() {
  int n = 0;
  while (*input_ptr < '0' || *input_ptr > '9') {
    ++input_ptr;
    if (input_ptr == input_ptr_end) {
      size_t read = fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
      if (read == 0) input_buffer[0] = '\0';
      input_ptr = input_buffer;
    }
  }
  while (*input_ptr >= '0' && *input_ptr <= '9') {
    n *= 10;
    n += *input_ptr - '0';
    ++input_ptr;
    if (input_ptr == input_ptr_end) {
      size_t read = fread(input_buffer, sizeof(char), BUFFER_SIZE - 1, stdin);
      if (read == 0) input_buffer[0] = '\0';
      input_ptr = input_buffer;
    }
  }
  return n;
}

int main() {

  scan_init();

  int n = scan_int();
  int m = scan_int();
  int k = scan_int();
  int s = scan_int();
  int t = scan_int();
  graph_t *graph = make_graph(n + 1);

  for (int i = 0; i < k; i++) {
    int tmp = scan_int();
    graph_add_airport(graph, tmp - 1);
  }
  for (int i = 0; i < m; i++) {
    int from = scan_int();
    int until = scan_int();
    graph_add_railway(graph, from - 1, until - 1);
  }

  int result = solve(graph, s - 1, t - 1);
  if (result == IMPOSSIBLE) {
    printf("Impossible\n");
  } else {
    printf("%d\n", result);
  }

  return 0;
}
