#include <tuple>
#include <type_traits>
#include <utility>
#include <cstddef>
#include <new>
#include <memory>
#include <string_view>

#include <boost/mp11.hpp>

#include "esp_log.h"

namespace cr {
    namespace mp = boost::mp11;

    // ========================= NTTP fixed_string & Tags =========================

    template<std::size_t N>
    struct fixed_string
    {
        char v[N]{};
        constexpr explicit fixed_string(char const (& s)[N]) { std::copy_n(s, N, v); }
    };

    inline constexpr auto default_name = fixed_string{"default"};

    template<fixed_string S> struct name_tag
    {
        static constexpr auto value = S;
    };

    template<class Dep, fixed_string S = default_name>
    struct qual
    {
        using dep = Dep;
        static constexpr auto value = S;
    };

    // ============================ DI-Declaration =================================

    template<class... Ts> struct depends_on
    {
        using list = mp::mp_list<Ts...>;
    };

    template<class T> struct component_traits
    {
        using deps = depends_on<>;
    }; // User-spezialisieren

    // =============================== Scope & Spec ================================

    enum class scope { singleton, prototype };

    // Vorwärtsdeklaration
    template<class T, fixed_string Name, scope Scope, class QualList, class ChildrenList, class... Args>
    struct spec;

    // Erkennungs-Traits für Parameter-Partitionierung
    template<class> struct is_name_tag : std::false_type {};
    template<fixed_string S> struct is_name_tag<name_tag<S> > : std::true_type {};
    template<class T> constexpr bool is_name_tag_v = is_name_tag<T>::value;

    template<class> struct is_qual : std::false_type {};
    template<class D, fixed_string S> struct is_qual<qual<D, S> > : std::true_type {};
    template<class T> constexpr bool is_qual_v = is_qual<T>::value;

    template<class> struct is_spec : std::false_type {};
    template<class T, fixed_string N, scope Sc, class Qs, class Cs, class... A>
    struct is_spec<spec<T, N, Sc, Qs, Cs, A...> > : std::true_type {};
    template<class T> constexpr bool is_spec_v = is_spec<T>::value;

    template<class T> constexpr bool is_arg_v = !(is_name_tag_v<T> || is_qual_v<T> || is_spec_v<T>);
    template<class T> struct is_arg : std::bool_constant<is_arg_v<T> > {};

    // Umwandlung mp_list<...> -> std::tuple<...>
    template<class L> struct mp_list_to_tuple;
    template<class... Xs>
    struct mp_list_to_tuple<mp::mp_list<Xs...> >
    {
        using type = std::tuple<Xs...>;
    };

    // Das Spec-Objekt hält:
    // - Name, Scope, Qualifier-Typeliste, Children-Typeliste
    // - die "Kinder-Spec-Objekte" (values)
    // - die partial User-Args (values)
    template<class T,
        fixed_string Name,
        scope Scope,
        class QualList,
        class ChildrenList,
        class... Args>
    struct spec
    {
        using type = T;
        static constexpr auto name = Name;
        static constexpr auto scope_v = Scope;
        using quals = QualList; // mp_list<qual<Dep,"id">...>
        using children = ChildrenList; // mp_list<spec<...>...>

        using children_tuple_t = mp_list_to_tuple<ChildrenList>::type;

        children_tuple_t children_specs{}; // echte Spec-Objekte der Kinder
        std::tuple<std::decay_t<Args>...> args{}; // partial User-Args
    };

    // ============================ Builder (singleton/prototype) ===================

    namespace detail {
        template<class...> struct first_name_or_default
        {
            static constexpr auto value = default_name;
        };
        template<class H, class... R>
        struct first_name_or_default<H, R...>
        {
            static constexpr auto value = [] {
                if constexpr (is_name_tag_v<std::remove_cvref_t<H> >) return std::remove_cvref_t<H>::value;
                else return first_name_or_default<R...>::value;
            }();
        };

        template<template<class> class F, class P>
        auto build_tuple_if(P&& p)
        {
            if constexpr (F<std::remove_cvref_t<P> >::value) return std::tuple{std::forward<P>(p)};
            else return std::tuple{};
        }

        // Baue ein Tuple aller Kindspezifikationen aus (P&&...) in Reihenfolge
        template<class... P>
        auto build_children_tuple(P&&... p)
        {
            return std::tuple_cat(build_tuple_if<is_spec>(std::forward<P>(p))...);
        }

        // Baue ein Tuple aller Args (alles was weder name_tag noch qual noch spec ist)
        template<class... P>
        auto build_args_tuple(P&&... p)
        {
            return std::tuple_cat(build_tuple_if<is_arg>(std::forward<P>(p))...);
        }

        template<class... P>
        using quals_list_t = mp::mp_copy_if<mp::mp_list<std::remove_cvref_t<P>...>, is_qual>;

        template<class... P>
        using children_list_t = mp::mp_copy_if<mp::mp_list<std::remove_cvref_t<P>...>, is_spec>;
    } // namespace detail

    template<class T, class... P>
    constexpr auto singleton(P&&... p)
    {
        using Qs = detail::quals_list_t<P...>;
        using Cs = detail::children_list_t<P...>;
        constexpr auto Nm = detail::first_name_or_default<P...>::value;

        return spec<T, Nm, scope::singleton, Qs, Cs, std::remove_cvref_t<P>...>{
            detail::build_children_tuple(std::forward<P>(p)...),
            detail::build_args_tuple(std::forward<P>(p)...)
        };
    }

    template<class T, class... P>
    constexpr auto prototype(P&&... p)
    {
        using Qs = detail::quals_list_t<P...>;
        using Cs = detail::children_list_t<P...>;
        constexpr auto Nm = detail::first_name_or_default<P...>::value;

        return spec<T, Nm, scope::prototype, Qs, Cs, std::remove_cvref_t<P>...>{
            detail::build_children_tuple(std::forward<P>(p)...),
            detail::build_args_tuple(std::forward<P>(p)...)
        };
    }

    // ============================== Keys / Utilities =============================

    template<class T, fixed_string Name>
    struct key
    {
        using type = T;
        static constexpr auto name = Name;
    };

    template<class S> struct key_of;
    template<class T, fixed_string N, scope Sc, class Qs, class Cs, class... A>
    struct key_of<spec<T, N, Sc, Qs, Cs, A...> >
    {
        using type = key<T, N>;
    };

    // ============================== Context (implizit) ===========================

    struct Context; // Vorwärts

    // ================================ Application ================================

    template<class... RootSpecs>
    struct Application;

    // Context hält einen Back-Ref auf Application für get<...>
    struct Context
    {
        Application<>* app_any = nullptr; // wir speichern untypisiert (Trick unten)

        // backref setzen (nur Application ruft das)
        void _attach(Application<>* a) noexcept { app_any = a; }

        // get<T> leitet an Application durch (nur Prototypen sinnvoll, aber auch Singletons möglich)
        template<class T, class... Runtime>
        decltype(auto) get(Runtime&&... rt);
    };

    // ------------------------------ Application ----------------------------------

    // Um Application<> untypisiert referenzieren zu können, definieren wir eine Basisklasse
    struct Application_base
    {
        virtual ~Application_base() = default;
    };

    template<class... RootSpecs>
    struct Application : Application_base
    {
        // Root-Specs speichern (inkl. Kinder / Quals / Args), und Slots nur für Singletons
        using roots_list = mp::mp_list<RootSpecs...>;

        // Flatten der Root-Specs beschränkt sich auf Root-Ebene.
        // Kinder liegen in den Slots der Owner (geschachtelt), daher keine globalen Keys nötig.

        // Slot für eine Spec: enthält Objekt + Kinderslots + Kinderspecs
        template<class Spec>
        struct slot_for
        {
            using T = Spec::type;
            alignas(T) std::byte buf[sizeof(T)]{};
            T* ptr{};

            using children_list = Spec::children;
            using children_slots_t = mp_list_to_tuple<mp::mp_transform<slot_for, children_list> >::type;

            children_slots_t child_slots{}; // Slots für Kinder
            Spec::children_tuple_t child_specs{}; // gespeicherte Kinder-Specs (Werte)
        };

        // Slots nur für Singletons; Prototypes haben keinen Slot
        template<class Spec> struct is_singleton : std::bool_constant<Spec::scope_v == scope::singleton> {};
        using singleton_roots_list = mp::mp_copy_if<roots_list, is_singleton>;

        template<class L> struct tuple_of_slots;
        template<class... S> struct tuple_of_slots<mp::mp_list<S...> >
        {
            using type = std::tuple<slot_for<S>...>;
        };
        using slots_t = tuple_of_slots<singleton_roots_list>::type;

        slots_t slots{};
        std::tuple<RootSpecs...> specs{}; // alle Roots (auch Prototypes), inklusive Kinder-Specs

        // Konstruktor kapselt Context: Der erste Parameterpaket-Teil sind Context-Args,
        // danach folgen RootSpecs... – wir bauen Context implicit als erstes Root-Singleton.
        explicit Application(RootSpecs... rs)
            : specs{std::forward<RootSpecs>(rs)...}
        {
            // Context-Slot existiert (erster Root ist Context Singleton). Merke Backref nach Konstruktion in run() beim ersten Zugriff.
        }

        // --------------------- Compile-time: Root-Indexing -------------------------

        // Baue Typeliste der Root-Keys
        using root_keys = mp::mp_transform<key_of, roots_list>;

        // Finde Root-Index nach (T,Name) – es muss genau eine geben
        template<class K, std::size_t I = 0>
        static consteval std::size_t find_root_index_impl()
        {
            static_assert(I < sizeof...(RootSpecs), "Key not found among root specs.");
            using KO = key_of<std::tuple_element_t<I, std::tuple<RootSpecs...> > >::type;
            if constexpr (std::is_same_v<K, KO>) return I;
            else return find_root_index_impl<K, I + 1>();
        }

        template<class T, fixed_string Name>
        static consteval std::size_t find_root_index()
        {
            return find_root_index_impl<key<T, Name> >();
        }

        // -------------------- Compile-time: Child-Indexing (Owner-lokal) ----------

        template<class OwnerSpec, class Dep, fixed_string Name, std::size_t I = 0>
        static consteval std::size_t find_child_index_impl()
        {
            using children = OwnerSpec::children;
            if constexpr (I >= mp::mp_size<children>::value) {
                static_assert(false, "Child spec not found in owner's children.");
                return 0;
            } else {
                using ChildS = mp::mp_at_c<children, I>;
                using KO = key_of<ChildS>::type;
                if constexpr (std::is_same_v<KO, key<Dep, Name> >) return I;
                else return find_child_index_impl<OwnerSpec, Dep, Name, I + 1>();
            }
        }

        // ---------------------- Sichtbarkeit / Auswahlregel ------------------------

        template<class Dep> struct spec_type_is_q
        {
            template<class Child> using fn = std::is_same<typename key_of<Child>::type::type, Dep>;
        };

        // Wie viele Kinder vom Typ Dep hat Owner?
        template<class OwnerSpec, class Dep>
        using owner_children_of_type = mp::mp_copy_if_q<
            typename OwnerSpec::children,
            spec_type_is_q<Dep>
        >;

        // Wie viele Root-Specs vom Typ Dep?
        template<class Dep>
        using root_of_type = mp::mp_copy_if_q<
            roots_list,
            spec_type_is_q<Dep>
        >;

        // Finde ggf. qualifizierten Namen in OwnerSpec::quals für Dep
        template<class OwnerSpec, class Dep>
        struct find_owner_qual_name
        {
            using qlist = OwnerSpec::quals;
            template<class Q> using is_q_for = std::is_same<typename Q::dep, Dep>;
            using matches = mp::mp_copy_if<qlist, is_q_for>;
            static constexpr bool has = mp::mp_size<matches>::value > 0;
            static constexpr auto value = [] {
                if constexpr (has) return mp::mp_front<matches>::value;
                else return default_name;
            }();
        };

        // Wähle für Dep die Quelle:
        // 1) Wenn Owner Kinder dieses Typs hat:
        //    - exakt 1 -> nimm das
        //    - >1      -> Owner muss qual<Dep,"id"> haben (Name aus qual)
        // 2) sonst Root:
        //    - exakt 1 -> nimm das
        //    - >1      -> require qual<Dep,"id">
        template<class OwnerSpec, class Dep>
        struct select_source
        {
            using kids = owner_children_of_type<OwnerSpec, Dep>;
            static constexpr std::size_t nk = mp::mp_size<kids>::value;
            using roots = root_of_type<Dep>;
            static constexpr std::size_t nr = mp::mp_size<roots>::value;

            static constexpr bool owner_has = (nk > 0);
            static constexpr bool root_has = (nr > 0);

            static_assert(owner_has || root_has, "Missing dependency: no component of the required type declared.");

            static constexpr bool owner_unique = (nk == 1);
            static constexpr bool root_unique = (nr == 1);

            static constexpr bool ambiguous =
                    (!owner_has && !root_unique) || // only root, but ambiguous
                    (owner_has && !owner_unique && !find_owner_qual_name<OwnerSpec, Dep>::has); // owner many, no qual

            static_assert(!ambiguous,
                          "Ambiguous dependency: multiple components of the required type; "
                          "specify a qualifier, e.g. qual<Dep, \"name\">{}.");

            // Result: kind = 0 -> owner/child; kind = 1 -> root
            static constexpr int kind = owner_has ? 0 : 1;
            // name to use (only used if >1 exist)
            static constexpr auto name =
                    (owner_has && !owner_unique) || (!owner_has && !root_unique)
                        ? find_owner_qual_name<OwnerSpec, Dep>::value
                        : default_name;
        };

        // ------------------------ Lazy DI + Cycle Check ----------------------------

        // slot_for für Root-Singletons per Index herausholen (Index im "singleton_roots_list")
        template<std::size_t I, class Path = mp::mp_list<> >
        void ensure_root_singleton() noexcept
        {
            auto& s = std::get<I>(slots);
            using RootS = mp::mp_at_c<singleton_roots_list, I>;
            using T = RootS::type;

            // Compile-time Zyklusprüfung: T darf nicht im Pfad sein
            static_assert(!mp::mp_contains<Path, T>::value, "Dependency cycle detected.");

            if (!s.ptr) {
                // erst Kinder (falls Root selbst Kinder hat)
                ensure_children_of<RootS, Path>();

                // dann Deps bauen
                auto deps = make_dep_tuple<T, RootS, Path>();
                // + partial args
                auto& spec_obj = get_root_spec_obj<RootS>();
                construct_from(deps, s, spec_obj.args);
            }
        }

        // Root-Spec-Objekt referenzieren (aus specs-Tuple)
        template<class RootS>
        RootS& get_root_spec_obj() noexcept
        {
            return get_root_spec_obj_impl<RootS, 0>();
        }

        template<class RootS, std::size_t I>
        RootS& get_root_spec_obj_impl() noexcept
        {
            if constexpr (I >= sizeof...(RootSpecs)) {
                // sollte nie passieren
                static_assert(false, "internal error: Root spec not found");
                abort();
            } else {
                using S = std::tuple_element_t<I, std::tuple<RootSpecs...> >;
                if constexpr (std::is_same_v<S, RootS>) {
                    return std::get<I>(specs);
                } else {
                    return get_root_spec_obj_impl<RootS, I + 1>();
                }
            }
        }

        // Kinder eines Owners sicherstellen (rekursiv)
        template<class OwnerSpec, class Path>
        void ensure_children_of() noexcept
        {
            using CL = OwnerSpec::children;
            ensure_children_impl<OwnerSpec, Path>(CL{});
        }

        template<class OwnerSpec, class Path, class... ChildS>
        void ensure_children_impl(mp::mp_list<ChildS...>) noexcept
        {
            auto& owner_slot = get_slot_for_owner<OwnerSpec>();
            auto& owner_spec = get_spec_for_owner<OwnerSpec>();
            ( ensure_child_one<OwnerSpec, ChildS, Path>(owner_slot, owner_spec), ... );
        }

        // Zugriff auf Owner-Root-Slot (nur für Root-Singletons sinnvoll)
        template<class OwnerSpec>
        auto& get_slot_for_owner() noexcept
        {
            // OwnerSpec muss Root sein und singleton
            constexpr std::size_t I = mp::mp_find<singleton_roots_list, OwnerSpec>::value;
            return std::get<I>(slots);
        }

        template<class OwnerSpec>
        OwnerSpec& get_spec_for_owner() noexcept
        {
            // OwnerSpec muss Root sein
            return get_root_spec_obj<OwnerSpec>();
        }

        // Ein Kind einer Root-Spec bauen
        template<class OwnerSpec, class ChildS, class Path>
        void ensure_child_one(auto& owner_slot, OwnerSpec& owner_spec) noexcept
        {
            // besorge Kind-Slot & Kind-Spec
            constexpr std::size_t idx =
                    mp::mp_find<typename OwnerSpec::children, ChildS>::value;
            auto& child_slot = std::get<idx>(owner_slot.child_slots);
            auto& child_spec = std::get<idx>(owner_spec.children_specs);
            using T = ChildS::type;

            static_assert(!mp::mp_contains<Path, T>::value, "Dependency cycle detected (child).");

            if (!child_slot.ptr) {
                // erst Kindeskinder
                ensure_children_of<ChildS, mp::mp_push_back<Path, T> >();

                // dann Deps
                auto deps = make_dep_tuple<T, ChildS, mp::mp_push_back<Path, T> >();
                construct_from(deps, child_slot, child_spec.args);
            }
        }

        // make_dep_tuple<T, OwnerSpec, Path> baut tuple<Deps&...> je nach Auswahlregel
        template<class T, class OwnerSpec, class Path>
        auto make_dep_tuple() noexcept
        {
            using DL = component_traits<T>::deps::list; // mp_list<Dep...>
            return make_dep_tuple_impl<T, OwnerSpec, Path>(DL{});
        }

        template<class T, class OwnerSpec, class Path, class... Ds>
        auto make_dep_tuple_impl(mp::mp_list<Ds...>) noexcept
        {
            return std::tuple<Ds&...>(get_dep_ref<OwnerSpec, Ds, Path>()...);
        }

        // get_dep_ref: wählt Quelle (child oder root), ggf. mit Qualifier
        template<class OwnerSpec, class Dep, class Path>
        Dep& get_dep_ref() noexcept
        {
            using sel = select_source<OwnerSpec, Dep>;
            if constexpr (sel::kind == 0) {
                // Kind
                if constexpr (mp::mp_size<owner_children_of_type<OwnerSpec, Dep> >::value == 1) {
                    // genau eines -> Name egal
                    constexpr std::size_t Idx =
                            find_child_index_impl<OwnerSpec, Dep, default_name>();
                    auto& owner_slot = get_slot_for_owner<OwnerSpec>();
                    auto& owner_spec = get_spec_for_owner<OwnerSpec>();
                    ensure_child_one<OwnerSpec, mp::mp_at_c<typename OwnerSpec::children, Idx>, Path>(
                        owner_slot, owner_spec);
                    auto& sl = std::get<Idx>(owner_slot.child_slots);
                    return *reinterpret_cast<Dep*>(sl.ptr);
                } else {
                    // mehrere -> Qualifier-Name ist gesetzt in sel::name
                    constexpr auto QN = sel::name;
                    constexpr std::size_t Idx =
                            find_child_index_impl<OwnerSpec, Dep, QN>();
                    auto& owner_slot = get_slot_for_owner<OwnerSpec>();
                    auto& owner_spec = get_spec_for_owner<OwnerSpec>();
                    ensure_child_one<OwnerSpec, mp::mp_at_c<typename OwnerSpec::children, Idx>, Path>(
                        owner_slot, owner_spec);
                    auto& sl = std::get<Idx>(owner_slot.child_slots);
                    return *reinterpret_cast<Dep*>(sl.ptr);
                }
            } else {
                // Root
                if constexpr (mp::mp_size<root_of_type<Dep> >::value == 1) {
                    // genau 1 -> nimm dessen einzigen Root-Key (Name egal)
                    // Finde den Root mit Typ Dep (egal Name); wir picken den ersten
                    constexpr std::size_t Ridx = find_root_by_type<Dep, 0>();
                    ensure_root_singleton<Ridx, Path>();
                    auto& sl = std::get<Ridx>(slots);
                    return *reinterpret_cast<Dep*>(sl.ptr);
                } else {
                    // mehrere Root -> qual Name erforderlich
                    constexpr auto QN = sel::name;
                    constexpr std::size_t Ridx = find_root_index<Dep, QN>();
                    // Ridx ist Index im RootSpecs... – aber wir brauchen den Index im "singleton_roots_list".
                    // Falls der gewählte Root ein Prototype ist -> Fehler (DI verlangt Singleton/Kind)
                    using PickedRootS = std::tuple_element_t<Ridx, std::tuple<RootSpecs...> >;
                    static_assert(PickedRootS::scope_v == scope::singleton,
                                  "Qualified dependency must be a singleton (or a child of the owner).");
                    constexpr std::size_t SlotIdx = mp::mp_find<singleton_roots_list, PickedRootS>::value;
                    ensure_root_singleton<SlotIdx, Path>();
                    auto& sl = std::get<SlotIdx>(slots);
                    return *reinterpret_cast<Dep*>(sl.ptr);
                }
            }
        }

        template<class Dep, std::size_t I>
        static consteval std::size_t find_root_by_type_impl()
        {
            if constexpr (I >= sizeof...(RootSpecs)) {
                static_assert(false, "No root component of requested type found.");
                return 0;
            } else {
                using RS = std::tuple_element_t<I, std::tuple<RootSpecs...> >;
                if constexpr (std::is_same_v<typename RS::type, Dep>) return I;
                else return find_root_by_type_impl<Dep, I + 1>();
            }
        }

        template<class Dep, std::size_t I = 0>
        static consteval std::size_t find_root_by_type()
        {
            return find_root_by_type_impl<Dep, I>();
        }

        // Konstruktion: Deps..., partial args...
        template<class DepTuple, class Slot, class ArgsTuple, std::size_t... Ds, std::size_t... As>
        void construct_apply(DepTuple& deps, Slot& sl, ArgsTuple& args,
                             std::index_sequence<Ds...>, std::index_sequence<As...>) noexcept
        {
            using T = Slot::T;
            void* p = sl.buf;
            T* obj = ::new(p) T(std::get<Ds>(deps)..., std::move(std::get<As>(args))...);
            sl.ptr = obj;
        }

        template<class DepTuple, class Slot, class ArgsTuple>
        void construct_from(DepTuple& deps, Slot& sl, ArgsTuple& args) noexcept
        {
            constexpr std::size_t ND = std::tuple_size_v<DepTuple>;
            constexpr std::size_t NA = std::tuple_size_v<ArgsTuple>;
            construct_apply(deps, sl, args,
                            std::make_index_sequence<ND>{},
                            std::make_index_sequence<NA>{});
        }

        // ----------------------------- Zerstörung ----------------------------------

        template<class S>
        void destroy_one_slot(slot_for<S>& sl) noexcept
        {
            using T = S::type;
            if (sl.ptr) {
                // erst Kinder zerstören (reverse Reihenfolge ist hier unkritisch, wir gehen flach)
                destroy_children_of<S>(sl);
                std::destroy_at(reinterpret_cast<T*>(sl.ptr));
                sl.ptr = nullptr;
            }
        }

        template<class S>
        void destroy_children_of(slot_for<S>& owner) noexcept
        {
            destroy_children_of_impl<S>(owner, typename S::children{});
        }

        template<class S, class... ChildS>
        void destroy_children_of_impl(slot_for<S>& owner, mp::mp_list<ChildS...>) noexcept
        {
            // reverse fold:
            ( destroy_one_slot(
                std::get<sizeof...(ChildS) - 1U - mp::mp_find<mp::mp_list<ChildS...>,
                             ChildS>::value>(owner.child_slots)), ... );
        }

    public:
        // ----------------------------- API -----------------------------------------

        // Konstruktion aller Singletons „on demand“ (hier optional vorab)
        void run() noexcept
        {
            construct_all_singletons();
            // Context backref setzen
            auto& ctx = get<Context>();
            //ctx._attach(static_cast<Application_base*>(this)); // pointer-compat
        }

        void stop() noexcept
        {
            destroy_all_singletons();
        }

        // get<T,Name>(runtime...)  – Name default "default"
        template<class T, fixed_string Name = default_name, class... Runtime>
        decltype(auto) get(Runtime&&... rt) noexcept
        {
            // finde Root-Spec T,Name
            constexpr std::size_t Ridx = find_root_index<T, Name>();
            using RootS = std::tuple_element_t<Ridx, std::tuple<RootSpecs...> >;
            if constexpr (RootS::scope_v == scope::singleton) {
                static_assert(sizeof...(Runtime) == 0,
                              "singleton<T>: runtime arguments are not allowed; pass all args in singleton<T>(...).");
                // Index in Singleton-Slots finden
                constexpr std::size_t SlotIdx = mp::mp_find<singleton_roots_list, RootS>::value;
                ensure_root_singleton<SlotIdx, mp::mp_list<> >();
                auto& sl = std::get<SlotIdx>(slots);
                using TT = RootS::type;
                return *reinterpret_cast<TT*>(sl.ptr);
            } else {
                // Prototype: baue on-the-fly
                using TT = RootS::type;
                // deps (Owner = RootS), partial = std::get<Ridx>(specs).args
                auto deps = make_dep_tuple<TT, RootS, mp::mp_list<> >();
                auto& spec_obj = std::get<Ridx>(specs);
                return construct_prototype<TT>(deps, spec_obj.args, std::forward<Runtime>(rt)...);
            }
        }

    private:
        // Prototypen bauen: Deps..., partial..., runtime...
        template<class T, class DepTuple, class PartialArgs, class... Runtime>
        T construct_prototype(DepTuple& deps, PartialArgs& partial, Runtime&&... rt) noexcept
        {
            return construct_prototype_impl<T>(deps, partial, std::forward<Runtime>(rt)...,
                                               std::make_index_sequence<std::tuple_size_v<DepTuple> >{},
                                               std::make_index_sequence<std::tuple_size_v<PartialArgs> >{});
        }

        template<class T, class DepTuple, class PartialArgs, class... Runtime, std::size_t... Ds, std::size_t... As>
        T construct_prototype_impl(DepTuple& deps, PartialArgs& par, Runtime&&... rt,
                                   std::index_sequence<Ds...>, std::index_sequence<As...>) noexcept
        {
            return T(std::get<Ds>(deps)..., std::get<As>(par)..., std::forward<Runtime>(rt)...);
        }

        // Alle Singletons (Root) konstruieren
        void construct_all_singletons() noexcept
        {
            construct_all_singletons_impl(std::make_index_sequence<mp::mp_size<singleton_roots_list>::value>{});
        }

        template<std::size_t... Is>
        void construct_all_singletons_impl(std::index_sequence<Is...>) noexcept
        {
            ( ensure_root_singleton<Is, mp::mp_list<> >(), ... );
        }

        void destroy_all_singletons() noexcept
        {
            destroy_all_singletons_impl(std::make_index_sequence<mp::mp_size<singleton_roots_list>::value>{});
        }

        template<std::size_t... Is>
        void destroy_all_singletons_impl(std::index_sequence<Is...>) noexcept
        {
            ( destroy_one_slot(std::get<sizeof...(Is) - 1U - Is>(slots)), ... );
        }
    };

    // Context::get -> ruft zur zugrunde liegenden Application (downcast aus Base)
    template<class T, class... Runtime>
    decltype(auto) Context::get(Runtime&&... rt)
    {
        auto* base = static_cast<Application_base*>(app_any);
        // Wir kennen die konkreten Template-Parameter von Application hier nicht.
        // Daher liefern wir get<T> ausschließlich über eine konkrete App-Instanz.
        // Praktisch: Context wird in derselben TU verwendet wie Application<...>,
        // und der Nutzer ruft app.get<Context>().get<T>(...) – dadurch ist der
        // statische Typ von 'this' = Context&, der wiederum weiß, welche App das ist.
        // Ein generisches Dispatch über Base ist hier nicht implementiert.
        // → Einfach: Nutzer ruft app.get<T>(...) direkt; oder Context hält
        //   eine get-Funktion-Objekt-Referenz. (Optional nachrüstbar.)
        // Für jetzt: liefern wir einen static_assert, falls jemand diesen Pfad nutzt.
        static_assert(!sizeof(T), "Use 'app.get<T>(...)' directly, or extend Context with app template.");
        return *this; // unreachable
    }

    // ============================== make_application =============================

    namespace detail {
        // Zähle vordere Context-Args (wir nehmen alle *nicht-spec* am Anfang),
        // danach folgen nur noch Specs
        template<class... P> struct split_ctx_and_specs;

        template<> struct split_ctx_and_specs<>
        {
            using ctx_tuple_t = std::tuple<>;
            using specs_list = mp::mp_list<>;
            static constexpr std::size_t num_ctx = 0;

            static ctx_tuple_t make_ctx_tuple() { return {}; }
            static auto make_specs_tuple() { return std::tuple<>(); }
        };

        template<class H, class... R>
        struct split_ctx_and_specs<H, R...>
        {
            using Hdec = std::remove_cvref_t<H>;
            static constexpr bool h_is_spec = is_spec<Hdec>::value;

            using rec = split_ctx_and_specs<R...>;

            using specs_list = std::conditional_t<
                h_is_spec,
                mp::mp_push_front<typename rec::specs_list, Hdec>,
                typename rec::specs_list
            >;

            static constexpr std::size_t num_ctx =
                    h_is_spec ? 0 : (1 + rec::num_ctx);

            static auto make_ctx_tuple(H&& h, R&&... r)
            {
                if constexpr (h_is_spec) {
                    return std::tuple<>();
                } else {
                    return std::tuple_cat(std::tuple{std::forward<H>(h)}, rec::make_ctx_tuple(std::forward<R>(r)...));
                }
            }
            static auto make_specs_tuple(H&& h, R&&... r)
            {
                if constexpr (h_is_spec) {
                    return std::tuple_cat(std::tuple{std::forward<H>(h)}, rec::make_specs_tuple(std::forward<R>(r)...));
                } else {
                    return rec::make_specs_tuple(std::forward<R>(r)...);
                }
            }
        };
    } // namespace detail

    template<class... P>
    auto make_application(P&&... p)
    {
        using splitter = detail::split_ctx_and_specs<P...>;
        auto ctx_tuple = splitter::make_ctx_tuple(std::forward<P>(p)...);
        auto specs_tuple = splitter::make_specs_tuple(std::forward<P>(p)...);

        auto ctx_spec = std::apply(
            []<typename... T>(T&&... a) { return singleton<Context>(std::forward<T>(a)...); },
            ctx_tuple
        );

        // tuple -> parameterpack
        return std::apply(
            [&]<typename... T>(T&&... ss) {
                using App = Application<std::decay_t<decltype(ctx_spec)>, std::decay_t<T>...>;
                return App{std::move(ctx_spec), std::forward<T>(ss)...};
            },
            std::move(specs_tuple)
        );
    }
} // namespace cr

class WiFi
{
public:
    WiFi(cr::Context& context)
    {
        ESP_LOGI("IoT", "WiFi constructor with contex=%p", &context);
    }
};

template<>
struct cr::component_traits<WiFi>
{
    using deps = depends_on<Context>;
};

class HttpClient
{
public:
    HttpClient(WiFi& wiFi, char const* url)
    {
        ESP_LOGI("IoT", "HttpClient constructor with WiFi=%p, url=%s", &wiFi, url);
    }
};

template<>
struct cr::component_traits<HttpClient>
{
    using deps = depends_on<WiFi>;
};

class Controller
{
public:
    Controller(HttpClient& client)
    {
        ESP_LOGI("IoT", "Controller constructor with HttpClient=%p", &client);
    }
};

template<> struct cr::component_traits<Controller>
{
    using deps = depends_on<HttpClient>;
};

auto app = cr::make_application(
    cr::singleton<WiFi>(),
    cr::singleton<Controller>(
        cr::singleton<HttpClient>("http://server:port")
    )
);

extern "C" void app_main()
{
    app.run();
}
