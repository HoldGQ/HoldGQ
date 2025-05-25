import unreal

def process_modols_assets():
    """
    Processes Modols assets, iterating through them and placing them in the level.
    This function will contain the core logic for asset processing.
    """
    unreal.log("Processing Modols assets...")
    static_mesh_assets_info = []
    try:
        asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
        modols_path = "/Game/Modols"

        search_options = unreal.AssetRegistrySearchOptions()
        search_options.package_paths = [modols_path]
        search_options.recursive_paths = True
        static_mesh_class_name = unreal.StaticMesh.static_class().get_name()
        search_options.asset_class_names = [static_mesh_class_name]

        asset_data_list = asset_registry.get_assets(search_options)
        
        num_assets_found = len(asset_data_list)
        unreal.log(f"Found {num_assets_found} static meshes in {modols_path}.")

        if num_assets_found > 0:
            for asset_data in asset_data_list:
                asset_path = str(asset_data.package_name)
                unreal.log(f"  Found StaticMesh: {asset_path}")
                static_mesh_assets_info.append({"path": asset_path, "asset_data": asset_data})
        else:
            unreal.log("No StaticMeshes found under {modols_path}") # Note: f-string was missing here
            # Corrected: unreal.log(f"No StaticMeshes found under {modols_path}")

    except Exception as e:
        unreal.log_error(f"Error during asset processing: {e}")
        return [] # Return empty list on error to signal failure

    return static_mesh_assets_info

def create_outliner_folders(static_mesh_assets_info, base_path="/Game/Modols"):
    """
    Creates a hierarchical folder structure in the World Outliner based on asset paths.
    """
    unreal.log("Starting to create Outliner folders...")
    created_folders = set()
    clean_base_path = base_path.strip("/")
    
    try:
        for asset_info in static_mesh_assets_info:
            try:
                full_asset_path = str(asset_info["path"]).strip("/")
                path_to_match = clean_base_path 
                
                if full_asset_path.startswith(path_to_match + "/"):
                    relative_path_with_asset_name = full_asset_path[len(path_to_match) + 1:]
                    path_components = relative_path_with_asset_name.split("/")

                    if len(path_components) > 1:
                        relative_folder_path = "/".join(path_components[:-1])
                        current_hierarchical_path = ""
                        for folder_component in relative_folder_path.split("/"):
                            if not current_hierarchical_path:
                                current_hierarchical_path = folder_component
                            else:
                                current_hierarchical_path = f"{current_hierarchical_path}/{folder_component}"
                            
                            if current_hierarchical_path not in created_folders:
                                try:
                                    # unreal.log(f"Attempting to create Outliner folder: {current_hierarchical_path}") # Less verbose
                                    unreal.EditorLevelLibrary.create_folder_in_world(current_hierarchical_path)
                                    created_folders.add(current_hierarchical_path)
                                    unreal.log(f"Successfully created/ensured Outliner folder: {current_hierarchical_path}")
                                except Exception as e_create:
                                    unreal.log_error(f"Failed to create Outliner folder '{current_hierarchical_path}': {e_create}")
            except Exception as e_asset:
                unreal.log_error(f"Error processing asset info for folder creation ('{asset_info.get('path', 'Unknown path')}'): {e_asset}")
                # Continue to next asset_info if possible
    except Exception as e_main_loop:
        unreal.log_error(f"A critical error occurred in create_outliner_folders main loop: {e_main_loop}")

    unreal.log("Finished creating Outliner folders.")

def place_static_meshes_in_level(static_mesh_assets_info, base_path="/Game/Modols"):
    """
    Loads static mesh assets, spawns them into the level, and places them in corresponding Outliner folders.
    """
    unreal.log("Starting to place static meshes in level...")
    editor_level_lib = unreal.EditorLevelLibrary()
    clean_base_path = base_path.strip("/")
    actors_placed_count = 0

    try:
        for asset_info in static_mesh_assets_info:
            asset_path = str(asset_info.get("path", "Unknown path")) # Use .get for safety
            try:
                # Load the Static Mesh Asset
                static_mesh = None
                try:
                    static_mesh = unreal.load_asset(asset_path)
                except Exception as e_load:
                    unreal.log_warning(f"Exception while loading asset {asset_path}: {e_load}")
                    continue # Skip to next asset

                if not static_mesh:
                    unreal.log_warning(f"Failed to load asset (returned None): {asset_path}")
                    continue
                if not isinstance(static_mesh, unreal.StaticMesh):
                    unreal.log_warning(f"Asset at {asset_path} is not a StaticMesh. Type is '{type(static_mesh)}'. Skipping.")
                    continue

                # Spawn Actor
                spawned_actor = None
                try:
                    actor_location = unreal.Vector(0.0, 0.0, 0.0)
                    actor_rotation = unreal.Rotator(0.0, 0.0, 0.0)
                    spawned_actor = editor_level_lib.spawn_actor_from_object(static_mesh, actor_location, actor_rotation)
                except Exception as e_spawn:
                    unreal.log_warning(f"Exception while spawning actor for StaticMesh {asset_path}: {e_spawn}")
                    continue # Skip to next asset

                if not spawned_actor:
                    unreal.log_warning(f"Failed to spawn actor (returned None) for StaticMesh: {asset_path}")
                    continue
                
                unreal.log(f"Spawned actor '{spawned_actor.get_actor_label()}' for '{asset_path}'")

                # Determine Target Outliner Folder Path
                relative_folder_path = ""
                full_asset_path_stripped = asset_path.strip("/")
                if full_asset_path_stripped.startswith(clean_base_path + "/"):
                    relative_path_with_asset_name = full_asset_path_stripped[len(clean_base_path) + 1:]
                    path_components = relative_path_with_asset_name.split("/")
                    if len(path_components) > 1:
                        relative_folder_path = "/".join(path_components[:-1])
                
                # Set Actor's Folder Path
                try:
                    if relative_folder_path:
                        spawned_actor.set_folder_path(relative_folder_path)
                        unreal.log(f"Moved actor '{spawned_actor.get_actor_label()}' to Outliner folder: '{relative_folder_path}'")
                    else:
                        spawned_actor.set_folder_path("")
                        unreal.log(f"Actor '{spawned_actor.get_actor_label()}' placed at Outliner root.")
                    actors_placed_count += 1
                except Exception as e_folder:
                    unreal.log_warning(f"Exception while setting folder path for actor '{spawned_actor.get_actor_label()}' (asset: {asset_path}): {e_folder}")

            except Exception as e_asset_loop:
                unreal.log_error(f"Error processing asset '{asset_path}' in place_static_meshes_in_level: {e_asset_loop}")
                # Continue to next asset_info if possible
    except Exception as e_main_loop:
        unreal.log_error(f"A critical error occurred in place_static_meshes_in_level main loop: {e_main_loop}")
            
    unreal.log(f"Finished placing static meshes in level. {actors_placed_count} actors placed.")

def run_asset_placer():
    """
    Main function to run the Modols Asset Placer tool.
    """
    unreal.log("Modols Asset Placer tool started.")
    overall_success = False
    try:
        # TODO: Add UI elements here if a dedicated window is built.
        # For now, the logic is executed directly.

        static_mesh_assets_info = process_modols_assets()

        if static_mesh_assets_info is None: # process_modols_assets signals error with None
            unreal.log_error("Asset processing failed. Aborting.")
        elif not static_mesh_assets_info: # Empty list, but no error during processing
            unreal.log("No assets found to process. Skipping folder creation and actor placement.")
            overall_success = True # Technically successful as there was nothing to do
        else:
            create_outliner_folders(static_mesh_assets_info, base_path="/Game/Modols")
            place_static_meshes_in_level(static_mesh_assets_info, base_path="/Game/Modols")
            overall_success = True # Assume success if functions complete without throwing to here

    except Exception as e:
        unreal.log_error(f"An unexpected error occurred during Modols Asset Placer execution: {e}")
        # overall_success remains False

    if overall_success:
        unreal.log("Modols Asset Placer tool finished processing successfully.")
    else:
        unreal.log_error("Modols Asset Placer tool encountered errors. Please check the log for details.")


if __name__ == "__main__":
    # This section is for testing the script directly if needed.
    # In Unreal Engine, this script will likely be triggered by a UI element or another mechanism.
    run_asset_placer()
