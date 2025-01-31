#ifndef SURREY_LOG_H
#define SURREY_LOG_H

#include <stdio.h>

// ANSI color codes
#define ANSI_COLOR_CLEAR_BLUE "\033[94m"
#define ANSI_COLOR_DEFAULT "\033[39m" // Reset to default text color
#define ANSI_COLOR_RESET "\033[0m" // Full reset of all attributes

// Define component types
typedef enum {
  COMP_E2_AGENT, // Changed from E2_AGENT
  COMP_RIC, // Changed from RIC
  COMP_XAPP, // Changed from XAPP
  COMP_SM // Changed from SM
} component_type_t;

// Function to get component string
static inline const char* get_component_string(component_type_t comp)
{
  switch (comp) {
    case COMP_E2_AGENT:
      return "E2-AGENT";
    case COMP_RIC:
      return "RIC";
    case COMP_XAPP:
      return "xAPP";
    case COMP_SM:
      return "SM";
    default:
      return "UNKNOWN";
  }
}

// Main macro for HiperSurrey logging with proper color reset
#define LOG_SURREY(format, ...)                                                                 \
  do {                                                                                          \
    printf(ANSI_COLOR_CLEAR_BLUE "[SURREY]   " format "%s", ##__VA_ARGS__, ANSI_COLOR_DEFAULT); \
    fflush(stdout);                                                                             \
  } while (0)

// Component-specific logging macro
#define LOG_SURREY_COMP(component, format, ...)                 \
  do {                                                          \
    printf(ANSI_COLOR_CLEAR_BLUE "[SURREY][%s]   " format "%s", \
           get_component_string(component),                     \
           ##__VA_ARGS__,                                       \
           ANSI_COLOR_DEFAULT);                                 \
    fflush(stdout);                                             \
  } while (0)

// Specific macros for each component
#define LOG_SURREY_E2AGENT(format, ...) LOG_SURREY_COMP(COMP_E2_AGENT, format, ##__VA_ARGS__)
#define LOG_SURREY_RIC(format, ...) LOG_SURREY_COMP(COMP_RIC, format, ##__VA_ARGS__)
#define LOG_SURREY_XAPP(format, ...) LOG_SURREY_COMP(COMP_XAPP, format, ##__VA_ARGS__)
#define LOG_SURREY_SM(format, ...) LOG_SURREY_COMP(COMP_SM, format, ##__VA_ARGS__)

// Optional: Conditional versions that can disable colors if needed
#ifdef NO_COLOR
#define LOG_SURREY_COND(format, ...) printf("[SURREY] " format, ##__VA_ARGS__)
#define LOG_SURREY_COMP_COND(component, format, ...) printf("[SURREY][%s] " format, get_component_string(component), ##__VA_ARGS__)
#endif

#endif // SURREY_LOG_H
