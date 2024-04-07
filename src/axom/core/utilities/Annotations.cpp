#include "Annotations.hpp"

#include "axom/core/utilities/About.hpp"
#include "axom/fmt.hpp"

#ifdef AXOM_USE_CALIPER
  #include "caliper/Caliper.h"
  #include "caliper/common/Attribute.h"
  #include "caliper/cali-manager.h"
  #ifdef AXOM_USE_MPI
    #include "caliper/cali-mpi.h"
  #endif
#endif

#ifdef AXOM_USE_ADIAK
  #include "adiak_tool.h"
#endif

#include <set>

#ifdef AXOM_USE_CALIPER
namespace cali
{
extern Attribute region_attr;
}
#endif

namespace axom
{
namespace utilities
{
namespace annotations
{
static bool adiak_initialized = false;

#ifdef AXOM_USE_CALIPER
static cali::ConfigManager *cali_mgr {nullptr};
#endif

namespace detail
{
#ifdef AXOM_USE_ADIAK
void initialize_common_adiak_metadata()
{
  adiak::user();
  adiak::launchdate();
  adiak::launchday();
  adiak::executable();
  adiak::clustername();
  adiak::cmdline();
  adiak::jobsize();
  adiak::numhosts();
  adiak::hostlist();
  adiak::workdir();

  adiak::walltime();
  adiak::systime();
  adiak::cputime();
}
#endif

#ifdef AXOM_USE_MPI
void initialize_adiak(MPI_Comm comm)
{
  #ifdef AXOM_USE_ADIAK
  if(adiak_initialized)
  {
    return;
  }

  adiak::init((void *)&comm);
  initialize_common_adiak_metadata();

  adiak_initialized = true;
  #else
  adiak_initialized = false;
  AXOM_UNUSED_VAR(comm);
  #endif
}
#else  // AXOM_USE_MPI
void initialize_adiak()
{
  #ifdef AXOM_USE_ADIAK
  if(adiak_initialized)
  {
    return;
  }

  adiak::init(nullptr);
  initialize_common_adiak_metadata();

  adiak_initialized = true;
  #else
  adiak_initialized = false;
  #endif
}
#endif  // AXOM_USE_MPI

void initialize_caliper(const std::string &mode, int num_ranks)
{
#ifdef AXOM_USE_CALIPER
  bool multiprocessing = (num_ranks > 1);
  std::string configuration_service_list;
  cali::ConfigManager::argmap_t app_args;

  #ifdef AXOM_USE_MPI
  cali_mpi_init();
  #endif

  cali_mgr = new cali::ConfigManager();
  cali_mgr->add(mode.c_str(), app_args);

  for(const auto &kv : app_args)
  {
    const std::string &cali_mode = kv.first;
    const std::string &value = kv.second;

    if(cali_mode == "none")
    {
      // intentionally empty
    }
    else if(cali_mode == "report")
    {
      //report is an alias for the runtime-report Caliper configuration
      cali_mgr->add("runtime-report(output=stdout,calc.inclusive=true)");
    }
    else if(cali_mode == "counts")
    {
      configuration_service_list = "event:aggregate:";
      configuration_service_list += multiprocessing ? "mpireport" : "report";

      cali_config_preset("CALI_REPORT_CONFIG",
                         "SELECT count() "
                         "GROUP BY prop:nested "
                         "WHERE cali.event.end "
                         "FORMAT tree");

      cali_config_preset("CALI_MPIREPORT_CONFIG",
                         "SELECT   min(count) as \"Min count\", "
                         "         max(count) as \"Max count\", "
                         "         avg(count) as \"Avg count\", "
                         "         sum(count) as \"Total count\" "
                         "GROUP BY prop:nested "
                         "WHERE    cali.event.end "
                         "FORMAT   tree");
    }
    else if(cali_mode == "file")
    {
      configuration_service_list = "event:aggregate:timestamp:recorder";
    }
    else if(cali_mode == "trace")
    {
      configuration_service_list = "event:trace:timestamp:recorder";
    }
    else if(cali_mode == "gputx")
    {
  #if defined(AXOM_USE_CUDA)
      configuration_service_list = "nvtx";
  #elif defined(AXOM_USE_HIP)
      configuration_service_list = "roctx";
  #endif
    }
    else if(cali_mode == "nvtx" || cali_mode == "nvprof")
    {
      configuration_service_list = "nvtx";
    }
    else if(cali_mode == "roctx")
    {
      configuration_service_list = "roctx";
    }
    else if(!value.empty())
    {
      declare_metadata(cali_mode, value);
    }
  }

  if(!configuration_service_list.empty())
  {
    if(multiprocessing)
    {
      //This ensures mpi appears before any related service
      configuration_service_list = "mpi:" + configuration_service_list;
    }
    cali_config_preset("CALI_TIMER_SNAPSHOT_DURATION", "true");
    cali_config_preset("CALI_TIMER_INCLUSIVE_DURATION", "false");
    cali_config_preset("CALI_SERVICES_ENABLE",
                       configuration_service_list.c_str());
  }

  for(auto &channel : cali_mgr->get_all_channels())
  {
    channel->start();
  }
#else
  AXOM_UNUSED_VAR(mode);
  AXOM_UNUSED_VAR(num_ranks);
#endif  // AXOM_USE_CALIPER
}

static const std::set<std::string> axom_valid_caliper_args = {"counts",
                                                              "file",
                                                              "gputx",
                                                              "none",
                                                              "nvprof",
                                                              "nvtx",
                                                              "report",
                                                              "trace",
                                                              "roctx"};

bool is_mode_valid(const std::string &mode)
{
#ifdef AXOM_USE_CALIPER
  cali::ConfigManager test_mgr;
  cali::ConfigManager::argmap_t app_args;

  const bool result = test_mgr.add(mode.c_str(), app_args);
  if(!result || test_mgr.error())
  {
    std::cerr << axom::fmt::format(
      "Bad caliper configuration for mode '{}' -> {}\n",
      mode,
      test_mgr.error_msg());
    return false;
  }

  for(const auto &kv : app_args)
  {
    const std::string &name = kv.first;
    const std::string &val = kv.second;

    if(!name.empty() && !val.empty())
    {
      continue;  // adiak-style NAME=VAL
    }
    if(axom_valid_caliper_args.find(name) != axom_valid_caliper_args.end())
    {
      continue;  // application argument
    }

    return false;
  }
  return true;
#else
  return (mode == "none") ? true : false;
#endif
}

std::string mode_help_string()
{
#ifdef AXOM_USE_CALIPER
  const auto built_in =
    axom::fmt::format("Built-in configurations: {}",
                      axom::fmt::join(axom_valid_caliper_args, ","));
  const auto cali_configs = axom::fmt::format(
    "Caliper configurations:\n{}",
    axom::fmt::join(cali::ConfigManager::get_config_docstrings(), "\n"));
  return built_in + "\n" + cali_configs;
#else
  return "Caliper not enabled at build-time, so the only valid mode is 'none'";
#endif
}

#ifdef AXOM_USE_ADIAK
static std::string adiak_value_as_string(adiak_value_t *val, adiak_datatype_t *t)
{
  // Implementation adapted from adiak user docs

  if(!t)
  {
    return "ERROR";
  }

  auto get_vals_array = [](adiak_datatype_t *t, adiak_value_t *val, int count) {
    std::vector<std::string> s;
    for(int i = 0; i < count; i++)
    {
      adiak_value_t subval;
      adiak_datatype_t *subtype;
      adiak_get_subval(t, val, i, &subtype, &subval);
      s.push_back(adiak_value_as_string(&subval, subtype));
    }
    return s;
  };

  switch(t->dtype)
  {
  case adiak_type_unset:
    return "UNSET";
  case adiak_long:
    return axom::fmt::format("{}", val->v_long);
  case adiak_ulong:
    return axom::fmt::format("{}", static_cast<unsigned long>(val->v_long));
  case adiak_longlong:
    return axom::fmt::format("{}", val->v_longlong);
  case adiak_ulonglong:
    return axom::fmt::format("{}",
                             static_cast<unsigned long long>(val->v_longlong));
  case adiak_int:
    return axom::fmt::format("{}", val->v_int);
  case adiak_uint:
    return axom::fmt::format("{}", static_cast<unsigned int>(val->v_int));
  case adiak_double:
    return axom::fmt::format("{}", val->v_double);
  case adiak_date:
    // holds time in seconds since epoch
    return axom::fmt::format(
      "{:%a %d %b %Y %T %z}",
      std::chrono::system_clock::time_point {std::chrono::seconds {val->v_long}});
  case adiak_timeval:
  {
    const auto *tv = static_cast<struct timeval *>(val->v_ptr);
    return axom::fmt::format("{:%S} seconds:timeval",
                             std::chrono::seconds {tv->tv_sec} +
                               std::chrono::microseconds {tv->tv_usec});
  }
  case adiak_version:
    return axom::fmt::format("{}:version", static_cast<char *>(val->v_ptr));
  case adiak_string:
    return axom::fmt::format("{}", static_cast<char *>(val->v_ptr));
  case adiak_catstring:
    return axom::fmt::format("{}:catstring", static_cast<char *>(val->v_ptr));
  case adiak_path:
    return axom::fmt::format("{}:path", static_cast<char *>(val->v_ptr));
  case adiak_range:
    return axom::fmt::format("{}",
                             axom::fmt::join(get_vals_array(t, val, 2), " - "));
  case adiak_set:
    return axom::fmt::format(
      "[{}]",
      axom::fmt::join(get_vals_array(t, val, adiak_num_subvals(t)), ", "));
  case adiak_list:
    return axom::fmt::format(
      "{{{}}}",
      axom::fmt::join(get_vals_array(t, val, adiak_num_subvals(t)), ", "));
  case adiak_tuple:
    return axom::fmt::format(
      "({})",
      axom::fmt::join(get_vals_array(t, val, adiak_num_subvals(t)), ", "));
  }
}

static void get_namevals_as_map(const char *name,
                                int AXOM_UNUSED_PARAM(category),
                                const char *AXOM_UNUSED_PARAM(subcategory),
                                adiak_value_t *value,
                                adiak_datatype_t *t,
                                void *opaque_value)
{
  // add each name/value to adiak metadata map
  using kv_map = std::map<std::string, std::string>;
  auto &metadata = *static_cast<kv_map *>(opaque_value);
  metadata[name] = adiak_value_as_string(value, t);
}

#endif  // AXOM_USE_ADIAK

}  // namespace detail

#ifdef AXOM_USE_MPI
void initialize(MPI_Comm comm, const std::string &mode)
{
  int num_ranks {1};
  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

  detail::initialize_adiak(comm);
  detail::initialize_caliper(mode, num_ranks);

  declare_metadata("axom_version", axom::getVersion());
}
#endif

void initialize(const std::string &mode)
{
  detail::initialize_adiak();
  detail::initialize_caliper(mode, 1);

  declare_metadata("axom_version", axom::getVersion());
}

void finalize()
{
#ifdef AXOM_USE_ADIAK
  if(adiak_initialized)
  {
    adiak::fini();
  }
  adiak_initialized = false;
#endif
#ifdef AXOM_USE_CALIPER
  if(cali_mgr)
  {
    for(auto &channel : cali_mgr->get_all_channels())
    {
      channel->flush();
    }

    delete cali_mgr;
    cali_mgr = nullptr;
  }
#endif
}

void begin(const std::string &name)
{
#ifdef AXOM_USE_CALIPER
  cali::Caliper().begin(
    cali::region_attr,
    cali::Variant(CALI_TYPE_STRING, name.c_str(), name.length()));
#else
  AXOM_UNUSED_VAR(name);
#endif
}

void end(const std::string & /*name*/)
{
#ifdef AXOM_USE_CALIPER
  cali::Caliper().end(cali::region_attr);
#endif
}

// returns registered adiak metadata as key-value pairs of strings
std::map<std::string, std::string> retrieve_metadata()
{
  std::map<std::string, std::string> metadata;

#ifdef AXOM_USE_ADIAK
  adiak_list_namevals(1, adiak_category_all, detail::get_namevals_as_map, &metadata);
#endif

  return metadata;
}

}  // namespace annotations
}  // namespace utilities
}  // namespace axom
