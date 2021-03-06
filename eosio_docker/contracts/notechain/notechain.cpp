#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
using namespace eosio;

#include <vector>



class dataChain : public eosio::contract {
  private:
    
    struct permission {
      std::string name;
      std::string description;
      bool state;
    };

    struct category{
      std::string name;
      std::string description;
      std::vector<permission> permissions;
    };

    //@abi table
    struct templ {
      uint64_t prim_key;
      account_name client;
      std::string client_name;
      std::string picture;
      std::vector<category> categories;

      auto primary_key() const { return prim_key; }
      account_name get_by_client() const { return client; }
    };

    typedef eosio::multi_index<N(templ), templ, 
    indexed_by<N(client), const_mem_fun<templ, account_name, &templ::get_by_client> > 
    > templtable;

    //@abi table
    struct thrust {
    
      uint64_t prim_key;
      account_name client;
      account_name user;
      bool dirty;
      std::vector<category> categories;
      std::string client_name;
      std::string picture;
    
      auto primary_key() const { return prim_key; }
      
      account_name get_by_client() const { return client; }
      account_name get_by_user() const { return user; }

    };

    // create a multi-index table and support secondary key
    typedef eosio::multi_index< N(thrust), thrust,
      indexed_by< N(user), const_mem_fun<thrust, account_name, &thrust::get_by_user> >,
      indexed_by< N(client), const_mem_fun<thrust, account_name, &thrust::get_by_client> >
      > thrusttable;

  public:
    using contract::contract;

    /// @abi action
    void settemplate( account_name _client, std::vector<category> _categories, std::string _client_name, std::string _picture ) {
      require_auth( _client );

      //check client exists
      templtable templates(_self, _self);

      auto client_index = templates.get_index<N(client)>();
      auto client_itr = client_index.find(_client);

      if(client_itr == client_index.end()){
        //create
        templates.emplace(_client, [&](auto &templ) {
          templ.prim_key = templates.available_primary_key();
          templ.client = _client;
          templ.categories = _categories;
          templ.client_name = _client_name;
          templ.picture = _picture;
        });
      }else{
        //update
        client_index.modify(client_itr, _client, [&](auto &templ) {
          templ.categories = _categories;
        });
        //flag thrust as dirty
        
        thrusttable thrusts(_self, _self);

        auto thrust_index = thrusts.get_index<N(client)>();
        auto thrust_itr = thrusts.find(_client);

        while(thrust_itr != thrusts.end()){

          thrusts.modify(thrust_itr, _client, [&](auto &thrust) {
            thrust.dirty = true;
          });

          thrust_itr++;
        } 

      }


    }

    void setperms( account_name _user, account_name _client, std::vector<category> _categories ) {
      require_auth( _user );

      thrusttable thrusts(_self, _self);

      auto thrust_index = thrusts.get_index<N(user)>();
      auto thrust_itr = thrust_index.find(_user);

      bool found = false;

      while(thrust_itr != thrust_index.end() && !found){

        if(thrust_itr->client == _client){ 
          thrust_index.modify(thrust_itr, _user, [&](auto &thrust) {
            thrust.dirty = false;
            thrust.categories = _categories;
          });
          found = true;
        }

        thrust_itr++;
      }

      if( !found ){

        std::string client_name = "";
        std::string picture = "";

        templtable templates(_self, _self);
        auto client_index = templates.get_index<N(client)>();
        auto client_itr = client_index.find(_client);

        if(client_itr != client_index.end()){
          client_name = client_itr->client_name;
          picture = client_itr->picture;
        }

        thrusts.emplace(_user, [&](auto &thrust) {
          thrust.prim_key = thrusts.available_primary_key();
          thrust.client = _client;
          thrust.user = _user;
          thrust.categories = _categories;
          thrust.dirty = false;
          thrust.client_name = client_name;
          thrust.picture = picture;
        });
      }

    }

};

// specify the contract name, and export a public action: update
EOSIO_ABI( dataChain, (settemplate) (setperms) )
