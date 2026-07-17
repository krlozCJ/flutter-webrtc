use crate::filter_manager::{send_command, FilterCommand};

pub fn add_filter(track_id: String, filter: String) -> bool {
    send_command(&track_id, FilterCommand::AddFilter(filter))
}

pub fn set_param(track_id: String, filter: String, param: String, value: f32) -> bool {
    send_command(&track_id, FilterCommand::SetParam { filter, param, value })
}